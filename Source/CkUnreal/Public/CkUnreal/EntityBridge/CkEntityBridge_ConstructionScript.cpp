#include "CkEntityBridge_ConstructionScript.h"

#include "CkEntityBridge_Fragment_Params.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

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
    if (NOT ShouldConstruct())
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto& OwningActor = GetOwner();

    if (OwningActor->bIsEditorOnlyActor)
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    Super::Do_Construct_Implementation(InParams);

    CK_ENSURE_IF_NOT(ck::IsValid(EntityConfig),
        TEXT("EntityConfig is [{}]. Did you forget to set the default value in the component?.[{}]"), EntityConfig, ck::Context(this))
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    if (OwningActor->GetIsReplicated() && GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        const auto EcsWorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

        // TODO: this hits at least once because the BP Subsystem is not loaded. Fix this.
        CK_ENSURE_IF_NOT(ck::IsValid(EcsWorldSubsystem),
            TEXT("WorldSubsystem is [{}]. Did you forget to set the default value in the component?.[{}]"), EcsWorldSubsystem, ck::Context(this))
        { return; }

        auto Entity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(EcsWorldSubsystem->Get_TransientEntity());

        Entity.Add<ck::FFragment_OwningActor_Current>(OwningActor);

        // --------------------------------------------------------------------------------------------------------------------
        // Add Net Connection Settings

        if (GetWorld()->IsNetMode(NM_Standalone))
        {
            UCk_Utils_Net_UE::Add(Entity, FCk_Net_ConnectionSettings{ECk_Net_NetModeType::Host, ECk_Net_EntityNetRole::Authority});
        }
        else if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
        {
            if (OwningActor->GetLocalRole() == ROLE_Authority)
            {
                UCk_Utils_Net_UE::Add(Entity, FCk_Net_ConnectionSettings{ECk_Net_NetModeType::Host, ECk_Net_EntityNetRole::Authority});
            }
        }
        else if (GetWorld()->IsNetMode(NM_Client))
        {
            if (OwningActor->GetLocalRole() == ROLE_Authority)
            {
                UCk_Utils_Net_UE::Add(Entity, FCk_Net_ConnectionSettings{ECk_Net_NetModeType::Client, ECk_Net_EntityNetRole::Authority});
            }
        }

        // --------------------------------------------------------------------------------------------------------------------
        // LINK TO ACTOR
        // EcsConstructionScript is a bit special in that it readies everything immediately instead of deferring the operation

        // We need the EntityOwningActor ActorComponent to exist before building the Entity from Config

        // TODO: consolidate into a utilty. Other usage in replication driver
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
                    true
                },
                [&](UCk_EntityOwningActor_ActorComponent_UE* InComp)
                {
                    InComp->_EntityHandle = Entity;
                }
            );
        }

        if (ck::Is_NOT_Valid(OwningActor->GetComponentByClass<UCk_ObjectReplicator_ActorComponent_UE>()))
        {
            UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_ActorComponent_UE>
            (
                UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_ActorComponent_UE>
                {
                    OwningActor,
                    true
                },
                [&](UCk_ObjectReplicator_ActorComponent_UE* InComp) { }
            );
        }

        // Add the replication driver here
        UCk_Utils_EntityReplicationDriver_UE::Add(Entity);

        // --------------------------------------------------------------------------------------------------------------------
        // Build Entity

        const auto CsWithTransform = Cast<UCk_EntityBridge_ConstructionScript_WithTransform_PDA>(EntityConfig->Get_EntityConstructionScript());

        CK_ENSURE_IF_NOT(ck::IsValid(CsWithTransform), TEXT("Entity Construction Script [{}] for Actor [{}] is NOT one with Transform. "
            "Entity Construction Scripts that have an Actor attached MUST use [{}]."), EntityConfig->Get_EntityConstructionScript(), OwningActor,
            ck::TypeToString<UCk_EntityBridge_ConstructionScript_WithTransform_PDA>)
        { return; }

        // TODO:
        CsWithTransform->Set_EntityInitialTransform(OwningActor->GetActorTransform());

        EntityConfig->Build(Entity);

        const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(Entity);
        UCk_Utils_EntityReplicationDriver_UE::Request_ReplicateEntityOnReplicatedActor(Entity,
            FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor
            {
                OwningActor, CsWithTransform
            }.Set_ReplicatedObjects(ReplicatedObjects.Get_ReplicatedObjects()));

        // TODO: Invoking this manually although ideally this should be called by ReplicationDriver for the Server
        TryInvoke_OnReplicationComplete(EInvoke_Caller::ReplicationDriver);
    }

    TryInvoke_OnReplicationComplete(EInvoke_Caller::EcsConstructionScript);
}

auto
    UCk_EntityBridge_ActorComponent_UE::
    TryInvoke_OnReplicationComplete(EInvoke_Caller InCaller)
    -> void
{
    switch (InCaller)
    {
        case EInvoke_Caller::ReplicationDriver:
        {

            if (_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Wait)
            { _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::WaitOnConstructionScript; }
            else
            {
                CK_ENSURE_IF_NOT(_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver,
                    TEXT("Expected BroadcastStep to be EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver"))
                { return; }

                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::Broadcast;
                _OnReplicationComplete_MC.Broadcast();
            }

            return;
        }
        case EInvoke_Caller::EcsConstructionScript:
        {
            if (_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Wait)
            { _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver; }
            else
            {
                CK_ENSURE_IF_NOT(_ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::WaitOnConstructionScript,
                    TEXT("Expected BroadcastStep to be EOnReplicationCompleteBroadcastStep::WaitOnReplicationDriver"))
                { return; }

                _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::Broadcast;
                _OnReplicationComplete_MC.Broadcast();
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
    Get_IsReplicationComplete() const
    -> bool
{
    // TODO: replace with Future once we have the feature
    return _ReplicationComplete_BroadcastStep == EOnReplicationCompleteBroadcastStep::Broadcast;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCKk_Utils_EntityBridge_ConstructionScript_UE::
    BuildEntity(
        FCk_Handle InHandle,
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityConfig),
        TEXT("InEntityConfig is INVALID. Cannot build Entity for Handle [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Handle is INVALID. Unable to build entity for [{}]"), InEntityConfig)
    { return {}; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    InEntityConfig->Build(NewEntity);

    return NewEntity;
}

// --------------------------------------------------------------------------------------------------------------------

