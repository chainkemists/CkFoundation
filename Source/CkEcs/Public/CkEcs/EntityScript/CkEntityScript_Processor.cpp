#include "CkEntityScript_Processor.h"

#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_SpawnEntity_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_ContinueConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_FinishConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_BeginPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_EndPlay);
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment)
        -> void
    {
        if (const auto& LifetimeOwner = InRequestFragment.Get_Owner();
            LifetimeOwner.Has_Any<FTag_EntityJustCreated, FTag_EntityScript_ContinueConstruction, FTag_EntityScript_FinishConstruction>())
        { return; }

        DoHandleRequest(InHandle, InRequestFragment);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityScript_SpawnEntity& InRequest)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(FCk_Request_EntityScript_SpawnEntity)
        const auto EntityScriptClassArchetype = InRequest.Get_EntityScriptClassArchetype();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScriptClassArchetype),
            TEXT("EntityScriptArchetype [{}] is INVALID. Cannot Spawn Entity"), EntityScriptClassArchetype)
        { return; }

        const auto& NewEntityScript = [&]() -> UCk_EntityScript_UE*
        {
            QUICK_SCOPE_CYCLE_COUNTER(FCk_Request_EntityScript_SpawnEntity_CreateObject)

            switch (EntityScriptClassArchetype->Get_InstancingPolicy())
            {
                case ECk_EntityScript_InstancingPolicy::NotInstanced:
                {
                    return EntityScriptClassArchetype.Get();
                }
                case ECk_EntityScript_InstancingPolicy::InstancedPerEntity:
                {
                    const auto& LifetimeOwner = InRequest.Get_Owner();
                    const auto& Outer = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(LifetimeOwner);

                    CK_ENSURE_IF_NOT(ck::IsValid(Outer),
                        TEXT("Failed to get valid World for Entity [{}] when trying to spawn EntityScript [{}]"),
                        LifetimeOwner,
                        EntityScriptClassArchetype)
                    { return {}; }

                    return UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EntityScript_UE>(Outer,
                        EntityScriptClassArchetype->GetClass(), EntityScriptClassArchetype.Get(), nullptr);
                }
                default:
                {
                    CK_INVALID_ENUM(EntityScriptClassArchetype->Get_InstancingPolicy());
                    return EntityScriptClassArchetype.Get();
                }
            }
        }();

        CK_ENSURE_IF_NOT(ck::IsValid(NewEntityScript),
            TEXT("Failed to Spawn New Entity using EntityScript [{}]"), EntityScriptClassArchetype)
        { return; }

        if (const auto& SpawnParams = InRequest.Get_SpawnParams();
            ck::IsValid(SpawnParams))
        {
            UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams(NewEntityScript, SpawnParams);
        }

        auto NewEntity = InRequest.Get_NewEntity();

        NewEntityScript->_AssociatedEntity = NewEntity;
        NewEntity.Add<FFragment_EntityScript_Current>(NewEntityScript);

        CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
            TEXT("EntityScript created: {}"), EntityScriptClassArchetype);

        switch (NewEntityScript->Construct(NewEntity, InRequest.Get_SpawnParams()))
        {
            case ECk_EntityScript_ConstructionFlow::Finished:
            {
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_EntityScript_UE, DoContinueConstruction);
                CK_ENSURE_IF_NOT(NOT NewEntityScript->GetClass()->IsFunctionImplementedInScript(ContinueConstructionFuncName),
                    TEXT("EntityScript [{}] Construction is FINISHED, but the script [{}] implements the [ContinueConstruction] event!\n"
                         "This event will be ignored as it is only invoked for ONGOING construction of EntityScript"),
                NewEntity, NewEntityScript) {}

                CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
                    TEXT("Construct() returned Finished"));
                NewEntity.Add<FTag_EntityScript_FinishConstruction>();
                break;
            }
            case ECk_EntityScript_ConstructionFlow::Continue:
            {
#if WITH_EDITOR
                const auto& IsBlueprintClass = ck::IsValid(NewEntityScript->GetClass()->ClassGeneratedBy);
#else
                // In non-editor builds, assume it's a Blueprint class
                const auto& IsBlueprintClass = true;
#endif
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_EntityScript_UE, DoContinueConstruction);
                CK_ENSURE_IF_NOT(NOT IsBlueprintClass || NewEntityScript->GetClass()->IsFunctionImplementedInScript(ContinueConstructionFuncName),
                    TEXT("EntityScript [{}] Construction is ONGOING, but the script [{}] DOES NOT implement the [ContinueConstruction] event!\n"
                         "Implement this event and ensure that [FinishConstruction] is called to ensure that the script correctly BeginPlay"),
                NewEntity, NewEntityScript) {}

                CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
                    TEXT("Construct() returned Continue"));
                NewEntity.Add<FTag_EntityScript_ContinueConstruction>();
                break;
            }
        }

        ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — Entity=[{}] Replication=[{}]"),
            NewEntity, NewEntityScript->Get_Replication());

        if (NewEntityScript->Get_Replication() == ECk_Replication::Replicates)
        {
            const auto HasOwningActor = UCk_Utils_OwningActor_UE::Has(NewEntity);
            ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — HasOwningActor=[{}]"), HasOwningActor);

            if (HasOwningActor)
            {
                auto* OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(NewEntity);
                auto* World = OwningActor->GetWorld();

                const auto IsClient = World->IsNetMode(NM_Client);

                ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — WithActor path, IsClient=[{}]"), IsClient);

                if (NOT IsClient)
                {
                    const auto NetMode = World->IsNetMode(NM_DedicatedServer)
                        ? ECk_Net_NetModeType::Host
                        : ECk_Net_NetModeType::ClientAndHost;

                    // Net params may have been pre-copied from the transient lifetime owner with
                    // DoesNotReplicate. Remove them so Add() can set the correct Replicates params.
                    // AddOrGet (used by Add) does NOT overwrite existing fragments.
                    if (NewEntity.Has<ck::FFragment_Net_Params>())
                    {
                        NewEntity.Remove<ck::FFragment_Net_Params>();
                    }
                    if (NewEntity.Has<ck::FTag_HasAuthority>())
                    {
                        NewEntity.Remove<ck::FTag_HasAuthority>();
                    }
                    if (NewEntity.Has<ck::FTag_NetMode_IsHost>())
                    {
                        NewEntity.Remove<ck::FTag_NetMode_IsHost>();
                    }
                    if (NewEntity.Has<ck::FTag_NetMode_IsClient>())
                    {
                        NewEntity.Remove<ck::FTag_NetMode_IsClient>();
                    }

                    UCk_Utils_Net_UE::Add(NewEntity, FCk_Net_ConnectionSettings{
                        ECk_Replication::Replicates, NetMode, ECk_Net_EntityNetRole::Authority});

                    auto* EntityOwningActorComponent =
                        OwningActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
                    EntityOwningActorComponent->Request_EnableReplication();

                    UCk_Utils_EntityReplicationDriver_UE::Add(NewEntity);

                    NewEntity.Add<ck::FRequest_EntityScript_Replicate>(
                        NewEntity, InRequest.Get_SpawnParams(), NewEntityScript);
                }
            }
            else
            {
                UCk_Utils_EntityReplicationDriver_UE::Add(NewEntity);

                auto ReplicatedOwner = InRequest.Get_Owner();
                const auto IsHost = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(ReplicatedOwner);
                ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — Non-WithActor path, Owner=[{}] IsHost=[{}]"),
                    ReplicatedOwner, IsHost);

                if (IsHost)
                {
                    NewEntity.Add<ck::FRequest_EntityScript_Replicate>(
                        ReplicatedOwner, InRequest.Get_SpawnParams(), NewEntityScript);
                }
                else
                {
                    ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — SKIPPED replication request (owner is not host)"));
                }
            }
        }
        else
        {
            ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — Entity does NOT replicate, skipping replication block"));
        }

        if (InRequest.Get_PostConstruction_Func())
        {
            InRequest.Get_PostConstruction_Func()(NewEntity);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_ContinueConstruction::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript),
            TEXT("EntityScript is INVALID for [{}] when attempting to invoke ContinueConstruction on it"), InHandle)
        { return; }

        CK_CALLSTACK_RECORD(ck::FFragment_EntityScript_Current, InHandle);
        EntityScript->ContinueConstruction(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_Replicate::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FRequest_EntityScript_Replicate& InRequest)
        -> void
    {
        const auto& EntityScript = InRequest.Get_Script();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript),
            TEXT("EntityScript is INVALID for [{}] when attempting to invoke ContinueConstruction on it"), InHandle)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        auto ReplicatedOwner = InRequest.Get_Owner();
        const auto IsSelfOwned = ReplicatedOwner == InHandle;

        ck::ecs::Display(TEXT("[REP_DEBUG] ReplicateProcessor — Handle=[{}] Owner=[{}] IsSelfOwned=[{}]"),
            InHandle, ReplicatedOwner, IsSelfOwned);

        if (NOT IsSelfOwned)
        {
            CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_Replication(ReplicatedOwner) == ECk_Replication::Replicates,
                TEXT("Attempting to Replicate newly spawned Entity Script [{}] with the Owner [{}] which is NOT Replicated!"),
                InHandle,
                ReplicatedOwner)
            {
                InHandle.Remove<MarkedDirtyBy>();
                return;
            }
        }

        if (NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
        {
            ck::ecs::Display(TEXT("[REP_DEBUG] ReplicateProcessor — Handle [{}] does NOT have ReplicationDriver yet, deferring"), InHandle);
            return;
        }

        const auto& OwnerReplicationDriver = IsSelfOwned
            ? InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()
            : ReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

        CK_ENSURE_IF_NOT(ck::IsValid(OwnerReplicationDriver),
            TEXT("Entity [{}] is missing a ReplicationDriver Fragment!"), ReplicatedOwner)
        {
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        ck::ecs::Display(TEXT("[REP_DEBUG] ReplicateProcessor — ReplicationDriver found, OwnerHasJustCreatedTag=[{}]"),
            ReplicatedOwner.Has<FTag_EntityJustCreated>());

        if (ReplicatedOwner.Has<FTag_EntityJustCreated>())
        {
            OwnerReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
                OwnerReplicationDriver->Get_ExpectedNumberOfDependentReplicationDrivers() + 1);
        }

        // NOTE: Copy on purpose otherwise the data doesn't make its way over correctly
        const auto SpawnParamsCopy = InRequest.Get_SpawnParams();

        ck::ecs::Display(TEXT("[REP_DEBUG] ReplicateProcessor — Calling Request_Replicate for [{}] with owner [{}]"), InHandle, ReplicatedOwner);

        UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(InHandle, ReplicatedOwner,
            InRequest.Get_Script()->GetClass(), SpawnParamsCopy);

        ck::ecs::Display(TEXT("[REP_DEBUG] ReplicateProcessor — Request_Replicate completed, adding FireOnDependentReplicationComplete tag"));

        InHandle.Add<FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_FinishConstruction::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InHandle,
            TEXT("Construction finished, ready for BeginPlay"));
        InHandle.Add<FTag_EntityScript_BeginPlay>();

        UUtils_Signal_OnConstructed::Broadcast(InHandle, ck::MakePayload(InHandle));

        if (ck::IsValid(InCurrent.Get_Script()))
        {
            auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

            if (ck::IsValid(LifetimeOwner) && LifetimeOwner.Has<FFragment_PendingReplication>())
            {
                auto& PendingFragment = LifetimeOwner.AddOrGet<FFragment_PendingReplication>();
                auto PendingEntity = PendingFragment.ConsumeFirst(InCurrent.Get_Script()->GetClass());

                if (ck::IsValid(PendingEntity))
                {
                    UUtils_Signal_OnConstructed::Broadcast(PendingEntity, ck::MakePayload(InHandle));
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntity);
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_BeginPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke BeginPlay on it"), InHandle)
        { return; }

        CK_CALLSTACK_RECORD(ck::FFragment_EntityScript_Current, InHandle);
        EntityScript->BeginPlay();

        InHandle.Add<FTag_EntityScript_HasBegunPlay>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_EndPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke EndPlay on it"), InHandle)
        { return; }

        CK_CALLSTACK_RECORD(ck::FFragment_EntityScript_Current, InHandle);
        EntityScript->EndPlay();
        InHandle.Add<FTag_EntityScript_HasEndedPlay>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

