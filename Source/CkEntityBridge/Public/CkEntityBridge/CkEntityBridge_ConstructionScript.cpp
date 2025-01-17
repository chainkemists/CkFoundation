#include "CkEntityBridge_ConstructionScript.h"

#include "CkEntityBridge_Fragment_Data.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
#include "CkEcs/CkEcsLog.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkEntityBridge/CkEntityBridge_Log.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_EntityBridge_ActorComponent_UE::
    UCk_EntityBridge_ActorComponent_UE()
{
    PrimaryComponentTick.bCanEverTick = false;
    bReplicateUsingRegisteredSubObjectList = true;
    SetIsReplicatedByDefault(true);
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    OnRegister()
    -> void
{
    Super::OnRegister();

    [this]()
    {
        if (IsTemplate())
        { return; }

        if (CreationMethod != EComponentCreationMethod::SimpleConstructionScript)
        { return; }

        if (UCk_Utils_Game_UE::Get_IsInGame(this))
        { return; }

        const auto& Outer = GetOuter();
        if (ck::Is_NOT_Valid(Outer))
        { return; }

        const auto& OuterClass = Outer->GetClass();
        if (ck::Is_NOT_Valid(OuterClass))
        { return; }

        const auto& OuterClassGeneratedByBlueprint = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(OuterClass);
        if (ck::Is_NOT_Valid(OuterClass))
        { return; }

        const auto& OuterBlueprint = Cast<UBlueprint>(OuterClassGeneratedByBlueprint);
        if (ck::Is_NOT_Valid(OuterBlueprint))
        { return; }

        UCk_Utils_EditorOnly_UE::Request_AddInterface(OuterBlueprint, UCk_Entity_ConstructionScript_Interface::StaticClass());
    }();
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    OnUnregister()
    -> void
{
    [this]()
    {
        if (IsTemplate())
        { return; }

        if (_Replication != ECk_Replication::Replicates)
        { return; }

        const auto& OwningActor = GetOwner();

        if (OwningActor->bIsEditorOnlyActor)
        { return; }

        if (const auto EntityOwningActorComponent = OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
            ck::IsValid(EntityOwningActorComponent))
        {
            auto OwningEntity = EntityOwningActorComponent->Get_EntityHandle();

            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(OwningEntity);

            if (NOT UCk_Utils_ReplicatedObjects_UE::Has(OwningEntity))
            { return; }

            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(OwningEntity);

            ck::algo::ForEachIsValid(ReplicatedObjects.Get_ReplicatedObjects(), [&](auto& InRO)
            {
                auto EcsRO = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO);
                UCk_Ecs_ReplicatedObject_UE::Destroy(EcsRO);
            });
        }
    }();

    Super::OnUnregister();
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams)
    -> void
{
    // --------------------------------------------------------------------------------------------------------------------

    const auto& OwningActor = GetOwner();

    if (OwningActor->bIsEditorOnlyActor)
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    Super::Do_Construct_Implementation(InParams);

    if (ck::Is_NOT_Valid(_ConstructionScript) && ck::IsValid(EntityConfig))
    {
        _ConstructionScript = EntityConfig->Get_EntityConstructionScript()->GetClass();
        ck::entity_bridge::Log(TEXT("[MIGRATE] EntityConfig with ConstructionScript [{}] migrated for [{}]"), _ConstructionScript, GetOwner());
        EntityConfig = nullptr;
    }

    CK_ENSURE_IF_NOT(ck::IsValid(_ConstructionScript),
        TEXT("EntityBridge ConstructionScript is [{}] for Actor [{}]. Did you forget to set the default value in the component?.[{}]"),
        _ConstructionScript,
        GetOwner(),
        ck::Context(this))
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    auto ConstructionScript = _ConstructionScript->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>();

    // --------------------------------------------------------------------------------------------------------------------

    //if (OwningActor->GetIsReplicated() && GetWorld()->IsNetMode(NM_DedicatedServer))
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        const auto EcsWorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

        CK_ENSURE_IF_NOT(ck::IsValid(EcsWorldSubsystem),
            TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"),
            EcsWorldSubsystem, ck::Context(this))
        { return; }

        auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(EcsWorldSubsystem);

        UCk_Utils_Handle_UE::Set_DebugName(Entity, UCk_Utils_Debug_UE::Get_DebugName(OwningActor, ECk_DebugNameVerbosity_Policy::Compact));
        UCk_Utils_OwningActor_UE::Add(Entity, OwningActor);

        // --------------------------------------------------------------------------------------------------------------------
        // Add Net Connection Settings

        UCk_Utils_Net_UE::Add(Entity, FCk_Net_ConnectionSettings{_Replication,
            ECk_Net_NetModeType::Host, ECk_Net_EntityNetRole::Authority});

        // --------------------------------------------------------------------------------------------------------------------
        // LINK TO ACTOR
        // EntityBridge is a bit special in that it readies everything immediately instead of deferring the operation

        // We need the EntityOwningActor ActorComponent to exist before building the Entity from Config

        // TODO: consolidate into a utility. Other usage in replication driver
        if (const auto EntityOwningActorComponent = OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
            ck::IsValid(EntityOwningActorComponent))
        {
            EntityOwningActorComponent->_EntityHandle = Entity;
        }
        else
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
                {
                    OwningActor,
                },
                [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
                {
                    InComp->_EntityHandle = Entity;
                }
            );
        }

        if (NOT UCk_Utils_Actor_UE::Get_HasComponent_ByClass(OwningActor, UCk_ObjectReplicator_ActorComponent_UE::StaticClass()))
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_ActorComponent_UE>
                {
                    OwningActor
                }
            );
        }

        // Add the replication driver here
        if (_Replication == ECk_Replication::Replicates)
        { UCk_Utils_EntityReplicationDriver_UE::Add(Entity); }

        // --------------------------------------------------------------------------------------------------------------------
        // Build Entity

        const auto CsWithTransform = Cast<UCk_Entity_ConstructionScript_WithTransform_PDA>(ConstructionScript);

        CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
            "Entity Construction Scripts that have an Actor attached MUST use [{}]."), ConstructionScript, OwningActor,
            ck::TypeToString<UCk_Entity_ConstructionScript_WithTransform_PDA>)
        { return; }

        CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

        TryInvoke_OnPreConstruct(Entity, EInvoke_Caller::EntityBridge);
        ConstructionScript->Construct(Entity, Get_EntityConstructionParamsToInject(), OwningActor);

        if (OwningActor->GetIsReplicated() && _Replication == ECk_Replication::Replicates)
        {
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(Entity);
            UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnReplicatedActor(Entity,
                FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor
                {
                    OwningActor, CsWithTransform
                }.Set_ReplicatedObjects(ReplicatedObjects.Get_ReplicatedObjects()));
        }

        // TODO: Invoking this manually although ideally this should be called by ReplicationDriver for the Server
        TryInvoke_OnReplicationComplete(EInvoke_Caller::ReplicationDriver);
    }
    else if (NOT OwningActor->GetIsReplicated() && GetWorld()->IsNetMode(NM_Client))
    {
        if (OwningActor->bIsEditorOnlyActor)
        { return; }

        auto NewEntity = [&]() -> FCk_Handle
        {
            switch(_Replication)
            {
                case ECk_Replication::Replicates:
                {
                    const auto OuterOwner = OwningActor->GetOwner();

                    CK_ENSURE_IF_NOT(ck::IsValid(OuterOwner),
                        TEXT("Unable to Replicate the Entity Associated with [{}]. Non-replicated Actors with Replicated Entities "
                            "MUST be spawned by a Replicated Actor.{}"),
                        OwningActor,
                        ck::Context(this))
                    { return {}; }

                    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(OuterOwner),
                        TEXT("Unable to Replicate the Entity Associated with [{}]. The OuterOwner [{}] is NOT ECS Ready. "
                            "This could happen if you have logic on BeginPlay (use our 'ReplicationComplete' events and Promises instead).{}"),
                        OwningActor,
                        OuterOwner,
                        ck::Context(this))
                    { return {}; }

                    const auto OuterOwnerEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(OuterOwner);

                    CK_ENSURE_IF_NOT(UCk_Utils_EntityReplicationDriver_UE::Has(OuterOwnerEntity),
                        TEXT("Unable to Replicate the Entity Associated with [{}]. The OuterOwner [{}] does NOT have a EntityReplicationDriver."
                            "This could happen if you have logic on BeginPlay (use our 'ReplicationComplete' events and Promises instead).{}"),
                        OwningActor,
                        OuterOwner,
                        ck::Context(this))
                    { return {}; }

                    CK_ENSURE_IF_NOT(ck::IsValid(OuterOwner->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>()),
                        TEXT("Unable to Replicate the Entity Associated with [{}]. The Outer [{}] of aforementioned does NOT  have an "
                            "ObjectReplicator ActorComponent.{}"),
                        OwningActor,
                        OuterOwner,
                        ck::Context(this))
                    { return {}; }

                    auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(OuterOwnerEntity, [&](FCk_Handle InEntity)
                    {
                        UCk_Utils_Handle_UE::Set_DebugName(InEntity, UCk_Utils_Debug_UE::Get_DebugName(OwningActor, ECk_DebugNameVerbosity_Policy::Compact));
                        UCk_Utils_OwningActor_UE::Add(InEntity, OwningActor);
                        UCk_Utils_Net_UE::Add
                        (
                            InEntity,
                            FCk_Net_ConnectionSettings
                            {
                                ECk_Replication::Replicates,
                                ECk_Net_NetModeType::Client,
                                OwningActor->GetLocalRole() == ROLE_AutonomousProxy ? ECk_Net_EntityNetRole::Authority : ECk_Net_EntityNetRole::Proxy
                            }
                        );
                    });

                    return Entity;
                }
                case ECk_Replication::DoesNotReplicate:
                {
                    auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(GetWorld(), [&](FCk_Handle InEntity)
                    {
                        UCk_Utils_Handle_UE::Set_DebugName(InEntity, UCk_Utils_Debug_UE::Get_DebugName(OwningActor, ECk_DebugNameVerbosity_Policy::Compact));
                        UCk_Utils_OwningActor_UE::Add(InEntity, OwningActor);

                        UCk_Utils_Net_UE::Add(InEntity, FCk_Net_ConnectionSettings{ECk_Replication::DoesNotReplicate,
                            ECk_Net_NetModeType::Client, ECk_Net_EntityNetRole::Authority});
                    });

                    return Entity;
                }
            }

            return {};
        }();

        CK_LOG_ERROR_IF_NOT(ck::entity_bridge, ck::IsValid(NewEntity),
            TEXT("Could NOT create a NewEntity for Actor [{}] based on previous errors"), OwningActor)
        { return; }

        // TODO: consolidate into a utility. Other usage in replication driver
        if (const auto EntityOwningActorComponent = OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
            ck::IsValid(EntityOwningActorComponent))
        {
            EntityOwningActorComponent->_EntityHandle = NewEntity;
        }
        else
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
                {
                    OwningActor
                },
                [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
                {
                    InComp->_EntityHandle = NewEntity;
                }
            );
        }

        // --------------------------------------------------------------------------------------------------------------------

        const auto CsWithTransform = Cast<UCk_Entity_ConstructionScript_WithTransform_PDA>(ConstructionScript);

        CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
            "Entity Construction Scripts that have an Actor attached MUST use [{}]."), ConstructionScript, OwningActor,
            ck::TypeToString<UCk_Entity_ConstructionScript_WithTransform_PDA>)
        { return; }

        CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

        TryInvoke_OnPreConstruct(NewEntity, EInvoke_Caller::EntityBridge);
        ConstructionScript->Construct(NewEntity, Get_EntityConstructionParamsToInject(), OwningActor);

        // TODO: this is a HACK due to the way TryInvoke_OnReplicationComplete works. The function assumes that it will be called twice.
        // Once by the EntityBridge and once by the ReplicationDriver. However, if we don't replicate at all, then there is no ReplicationDriver
        // and we need to 'spoof' it
        if (_Replication == ECk_Replication::DoesNotReplicate)
        { TryInvoke_OnReplicationComplete(EInvoke_Caller::ReplicationDriver); }
    }
    else if (OwningActor->GetIsReplicated() && _Replication == ECk_Replication::DoesNotReplicate)
    {
        if (OwningActor->bIsEditorOnlyActor)
        { return; }

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(GetWorld(), [&](FCk_Handle InEntity)
        {
            UCk_Utils_Handle_UE::Set_DebugName(InEntity, UCk_Utils_Debug_UE::Get_DebugName(OwningActor, ECk_DebugNameVerbosity_Policy::Compact));
            UCk_Utils_OwningActor_UE::Add(InEntity, OwningActor);

            UCk_Utils_Net_UE::Add(InEntity, FCk_Net_ConnectionSettings{ECk_Replication::DoesNotReplicate,
                ECk_Net_NetModeType::Client, ECk_Net_EntityNetRole::Authority});
        });

        CK_LOG_ERROR_IF_NOT(ck::entity_bridge, ck::IsValid(NewEntity),
            TEXT("Could NOT create a NewEntity for Non-Replicated Actor [{}] based on previous errors"), OwningActor)
        { return; }

        // TODO: consolidate into a utility. Other usage in replication driver
        if (const auto EntityOwningActorComponent = OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
            ck::IsValid(EntityOwningActorComponent))
        {
            EntityOwningActorComponent->_EntityHandle = NewEntity;
        }
        else
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
                {
                    OwningActor
                },
                [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
                {
                    InComp->_EntityHandle = NewEntity;
                }
            );
        }

        // --------------------------------------------------------------------------------------------------------------------

        const auto CsWithTransform = Cast<UCk_Entity_ConstructionScript_WithTransform_PDA>(ConstructionScript);

        CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
            "Entity Construction Scripts that have an Actor attached MUST use [{}]."), ConstructionScript, OwningActor,
            ck::TypeToString<UCk_Entity_ConstructionScript_WithTransform_PDA>)
        { return; }

        CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

        TryInvoke_OnPreConstruct(NewEntity, EInvoke_Caller::EntityBridge);
        ConstructionScript->Construct(NewEntity, Get_EntityConstructionParamsToInject(), OwningActor);

        // TODO: this is a HACK due to the way TryInvoke_OnReplicationComplete works. The function assumes that it will be called twice.
        // Once by the EntityBridge and once by the ReplicationDriver. However, if we don't replicate at all, then there is no ReplicationDriver
        // and we need to 'spoof' it
        TryInvoke_OnReplicationComplete(EInvoke_Caller::ReplicationDriver);
    }

    TryInvoke_OnReplicationComplete(EInvoke_Caller::EntityBridge);
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    TryInvoke_OnPreConstruct(
        const FCk_Handle& Entity,
        EInvoke_Caller InCaller) const
    -> void
{
    _OnPreConstruct.Broadcast(Entity);
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    TryInvoke_OnReplicationComplete(
        EInvoke_Caller InCaller)
    -> void
{
    switch (InCaller)
    {
        case EInvoke_Caller::ReplicationDriver:
        {
            if (_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Wait)
            {
                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::WaitOnConstructionScript;
            }
            else
            {
                CK_ENSURE_IF_NOT(_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver,
                    TEXT("Expected BroadcastStep to be EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver"))
                { return; }

                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::Broadcast;

                const auto AssociatedEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(GetOwner());
                _OnReplicationComplete_MC.Broadcast(AssociatedEntity);
            }

            return;
        }
        case EInvoke_Caller::EntityBridge:
        {
            if (_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Wait)
            {
                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver;
            }
            else
            {
                CK_ENSURE_IF_NOT(_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::WaitOnConstructionScript,
                    TEXT("Expected BroadcastStep to be EOnReplicationCompleteBroadcastStep::WaitOnConstructionScript"))
                { return; }

                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::Broadcast;

                const auto AssociatedEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(GetOwner());
                _OnReplicationComplete_MC.Broadcast(AssociatedEntity);
            }
            break;
        }
        default:
        {
            CK_TRIGGER_ENSURE(TEXT("Invalid enum value for Enum [{}]"), ck::TypeToString<EInvoke_Caller>);
            return;
        }
    }
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    Get_EntityConstructionParamsToInject() const
    -> FInstancedStruct
{
    const FCk_SharedInstancedStruct ParamsToInject;
    _OnInjectEntityConstructionParams_MC.Broadcast(ParamsToInject);

    return *ParamsToInject;
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    Request_ReplicateNonReplicatedActor()
    -> void
{
    const auto& OwningActor = GetOwner();

    if (OwningActor->bIsEditorOnlyActor)
    { return; }

    const auto OuterOwner = OwningActor->GetOwner();

    CK_ENSURE_IF_NOT(ck::IsValid(OuterOwner),
        TEXT("Unable to Replicate the Entity Associated with [{}]. Non-replicated Actors with Replicated Entities "
            "MUST be spawned by a Replicated Actor.{}"),
        OwningActor,
        ck::Context(this))
    { return; }

    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(OwningActor),
        TEXT("Unable to Replicate the Entity Associated with [{}]. The Actor is NOT ECS Ready. "
            "This could happen if you have logic on BeginPlay (use our 'ReplicationComplete' events and Promises instead).{}"),
        OwningActor,
        ck::Context(this))
    { return; }

    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(OuterOwner),
        TEXT("Unable to Replicate the Entity Associated with [{}]. The OuterOwner [{}] is NOT ECS Ready. "
            "This could happen if you have logic on BeginPlay (use our 'ReplicationComplete' events and Promises instead).{}"),
        OwningActor,
        OuterOwner,
        ck::Context(this))
    { return; }

    auto OuterOwnerEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(OuterOwner);

    CK_ENSURE_IF_NOT(UCk_Utils_EntityReplicationDriver_UE::Has(OuterOwnerEntity),
        TEXT("Unable to Replicate the Entity Associated with [{}]. The OuterOwner [{}] does NOT have a EntityReplicationDriver."
            "This could happen if you have logic on BeginPlay (use our 'ReplicationComplete' events and Promises instead).{}"),
        OwningActor,
        OuterOwner,
        ck::Context(this))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(OuterOwner->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>()),
        TEXT("Unable to Replicate the Entity Associated with [{}]. The Outer [{}] of aforementioned does NOT  have an "
            "ObjectReplicator ActorComponent.{}"),
        OwningActor,
        OuterOwner,
        ck::Context(this))
    { return; }

    const auto NewEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(OwningActor);

    UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnNonReplicatedActor(OuterOwnerEntity,
        FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor
        {
            OuterOwner,
            OwningActor->GetClass(),
            static_cast<int32>(NewEntity.Get_Entity().Get_ID()),
            OwningActor->GetWorld()->GetFirstPlayerController()
        }.Set_StartingLocation(OwningActor->GetActorLocation()));
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    Get_IsReplicationComplete() const
    -> bool
{
    // TODO: replace with Future once we have the feature
    return _ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Broadcast;
}

// --------------------------------------------------------------------------------------------------------------------

