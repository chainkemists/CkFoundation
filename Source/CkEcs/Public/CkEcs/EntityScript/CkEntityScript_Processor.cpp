#include "CkEntityScript_Processor.h"

#include "CkEntityScript.h"
#include "CkEntityScript_Utils.h"
#include "CkGenericEntityScript.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // FTag_DestroyEntity_* (owner-liveness cancel)
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h" // FFragment_SpawnRecipe retention (RuntimeSpawned)
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Tag/CkTag_EditorOnly.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_SpawnEntity_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_SpawnEntity_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_ContinueConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_FinishConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_PendingReplicationRetry);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_BeginPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_EndPlay);
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_spawn_trace
{
    static TAutoConsoleVariable<int32>
        CVar_LogSpawnRequestVisits(
            TEXT("ck.EntityScript.LogSpawnRequestVisits"),
            0,
            TEXT(
                "Logs every EntityScript spawn-request visit with its class, "
                "owner, new entity, request entity, frame, and disposition. "
                "Diagnostic only; leave disabled during performance capture."),
            ECVF_Default);

    auto
    LogVisit(
        const TCHAR* InDisposition,
        const FCk_Handle& InRequestEntity,
        const FCk_Request_EntityScript_SpawnEntity&
            InRequest)
        -> void
    {
        if (CVar_LogSpawnRequestVisits
                .GetValueOnGameThread()
            <= 0)
        { return; }

        ck::ecs::Display(
            TEXT(
                "[EntityScript SpawnTrace] frame=[{}] disposition=[{}] "
                "script=[{}] entity=[{}] owner=[{}] request=[{}]"),
            GFrameCounter,
            InDisposition,
            InRequest.Get_EntityScriptClassArchetype(),
            InRequest.Get_NewEntity(),
            InRequest.Get_Owner(),
            InRequestEntity);
    }
}

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
        // A dying owner means this spawn lost the race with its owner's destroy cascade; cancel rather than
        // materialize a zombie child that outlives its owner. Must run BEFORE the mid-construction defer below
        // (else an owner dying mid-construction defers forever), and Is_NOT_Valid must short-circuit the Has
        // queries — a finalized owner is a tombstone handle and any Has query on it would itself ensure.
        if (const auto& LifetimeOwner = InRequestFragment.Get_Owner();
            ck::Is_NOT_Valid(LifetimeOwner) ||
            LifetimeOwner.Has_Any<FTag_DestroyEntity_Initiate, FTag_DestroyEntity_EndPlay,
                FTag_DestroyEntity_Teardown, FTag_DestroyEntity_Await, FTag_DestroyEntity_Finalize>())
        {
            ck_entity_script_spawn_trace::LogVisit(
                TEXT("cancelled"),
                InHandle,
                InRequestFragment);

            ck::ecs::Verbose(TEXT("Cancelling EntityScript spawn of [{}] — lifetime owner [{}] is destroyed or "
                "pending-destroy (owner cascade applied late)"),
                InRequestFragment.Get_EntityScriptClassArchetype(), LifetimeOwner);

            InRequestFragment.TryFireCompletion(InRequestFragment.Get_NewEntity(),
                ECk_Request_OperationResult::Failed_Cancelled);

            if (auto OrphanedEntity = InRequestFragment.Get_NewEntity();
                ck::IsValid(OrphanedEntity))
            { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(OrphanedEntity); }

            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        // A child EntityScript spawned from inside a parent's Construct callback must wait for the parent to
        // finish its own pipeline — these two tags are the true "construction in progress" markers.
        if (const auto& LifetimeOwner = InRequestFragment.Get_Owner();
            LifetimeOwner.Has_Any<FTag_EntityScript_ContinueConstruction, FTag_EntityScript_FinishConstruction>())
        {
            ck_entity_script_spawn_trace::LogVisit(
                TEXT("deferred"),
                InHandle,
                InRequestFragment);
            return;
        }

        ck_entity_script_spawn_trace::LogVisit(
            TEXT("execute"),
            InHandle,
            InRequestFragment);
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

        // The guard holds Result by reference and fires at scope exit, so it must stay declared after it.
        // Every early-out below is a genuine failure to spawn; reaching the end IS the success condition.
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InRequest.Get_NewEntity(), Result);

        const auto EntityScriptClassArchetype = InRequest.Get_EntityScriptClassArchetype();

#if WITH_EDITOR
        // Editor-preview spawns pass the spawner's INSTANCED script as the archetype (runtime spawns pass a CDO,
        // which cannot die), and drag-drop destroys the preview actor — taking its script — after the request was
        // enqueued. Expected churn, not an error: drop silently. The runtime ensure below stays reachable.
        if (ck::Is_NOT_Valid(EntityScriptClassArchetype) && InHandle.Has<FTag_EditorOnlyEntity>())
        {
            ck::ecs::Verbose(TEXT("Dropping editor-only spawn request [{}] — instanced archetype died before processing (drag-drop preview churn)"), InHandle);

            if (auto OrphanedEntity = InRequest.Get_NewEntity();
                ck::IsValid(OrphanedEntity))
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(OrphanedEntity);
            }
            return;
        }
#endif

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
                case ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable:
                {
                    const auto& LifetimeOwner = InRequest.Get_Owner();
                    const auto& Outer = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(LifetimeOwner);

                    CK_ENSURE_IF_NOT(ck::IsValid(Outer),
                        TEXT("Failed to get valid World for Entity [{}] when trying to spawn EntityScript [{}]"),
                        LifetimeOwner,
                        EntityScriptClassArchetype)
                    { return {}; }

                    // both instanced policies go through the pooling subsystem (fragment holds a weak ptr):
                    // Poolable recycles, plain InstancedPerEntity is pinned-unique. Spawn params inject below
                    const auto IsPoolable = EntityScriptClassArchetype->Get_InstancingPolicy() ==
                        ECk_EntityScript_InstancingPolicy::InstancedPerEntity_Poolable;

                    const auto PoolParams = IsPoolable
                        ? FCk_ObjectPooling_PoolParams{EntityScriptClassArchetype->Get_PoolParams()}
                            .Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::Recycle)
                        : FCk_ObjectPooling_PoolParams{}
                            .Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease);

                    return UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EntityScript_UE>(Outer,
                        EntityScriptClassArchetype->GetClass(), EntityScriptClassArchetype.Get(),
                        PoolParams, nullptr);
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

        // ---- Retain the construction recipe (save/load rebuild+hydrate, RuntimeSpawned) ----------
        // The recipe is pinned by a GC-safe holder UObject because fragments are not GC-traced. Always stamped;
        // the save capture filters by provenance, so entities that carry one needlessly never consult it.
        if (const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(NewEntity);
            ck::IsValid(World))
        {
            auto* Recipe = NewObject<UCk_EntityScript_SpawnRecipe_UE>(World);
            Recipe->Populate(EntityScriptClassArchetype->GetClass(), InRequest.Get_SpawnParams());
            NewEntity.Add<ck::FFragment_SpawnRecipe>(TStrongObjectPtr<UCk_EntityScript_SpawnRecipe_UE>{Recipe});
        }

        ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] SpawnProcessor — Entity=[{}] Replication=[{}]"),
            NewEntity, NewEntityScript->Get_EffectiveReplication());

        // Net_Params is established upstream in UCk_Utils_EntityScript_UE::Add; this ensure catches a future
        // caller that enqueues a spawn request while bypassing Add.
        CK_ENSURE_IF_NOT(NewEntity.Has<ck::FFragment_Net_Params>(),
            TEXT("Entity [{}] is missing NetParams before construction. EntityScript: [{}]. "
                 "Did a caller enqueue FFragment_EntityScript_RequestSpawnEntity directly instead "
                 "of going through UCk_Utils_EntityScript_UE::Add?"),
            NewEntity, EntityScriptClassArchetype) {}

        // ---- Replication Driver (before Construct) ----------------------------------------
        // Non-actor-bearing replicated entities need their driver present BEFORE Construct so utilities called
        // during Construct find it. Entities that receive their own OwningActor later during Construct (WithActor)
        // have no actor in the chain yet — TryAdd fails gracefully and OwningActor::Add supplies the driver.
        UCk_Utils_EntityReplicationDriver_UE::TryAdd(NewEntity);

        // ---- Construct --------------------------------------------------------------------
        switch (NewEntityScript->Construct(NewEntity, InRequest.Get_SpawnParams()))
        {
            case ECk_EntityScript_ConstructionFlow::Finished:
            {
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_GenericEntityScript_UE, DoContinueConstruction);
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
                CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
                    TEXT("Construct() returned Continue"));
                NewEntity.Add<FTag_EntityScript_ContinueConstruction>();
                break;
            }
        }

        // ---- Replication Infrastructure (after Construct) ---------------------------------
        // The driver already exists — added pre-Construct, or during Construct via UCk_Utils_OwningActor_UE::Add.
        if (NewEntityScript->Get_EffectiveReplication() == ECk_Replication::Replicates)
        {
            // The entity's live ContextOwner fragment is authoritative: it reflects the inherited value AND any
            // retarget the spawner applied. Leave the override unset when the entity is its own ContextOwner —
            // OnRep then maps it back to self on the client rather than to the replicated lifetime-owner.
            auto ContextOwnerOverride = TOptional<FCk_Handle>{};
            if (UCk_Utils_ContextOwner_UE::Has(NewEntity))
            {
                if (const auto EntityContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(NewEntity);
                    EntityContextOwner != NewEntity)
                { ContextOwnerOverride = EntityContextOwner; }
            }

            const auto HasOwningActor = UCk_Utils_OwningActor_UE::Has(NewEntity);
            ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] SpawnProcessor — HasOwningActor=[{}]"), HasOwningActor);

            if (HasOwningActor)
            {
                const auto* OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(NewEntity);
                const auto* World = OwningActor->GetWorld();

                const auto IsNetworkedAuthority =
                    World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);

                ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] SpawnProcessor — WithActor path, IsNetworkedAuthority=[{}]"),
                    IsNetworkedAuthority);

                // Only on a networked authority: NM_Standalone has no net driver, and on NM_Client
                // authority-side replication is not our concern.
                if (IsNetworkedAuthority)
                {
                    NewEntity.Add<ck::FRequest_EntityScript_Replicate>(
                        NewEntity, ContextOwnerOverride, InRequest.Get_SpawnParams(), NewEntityScript);
                }
            }
            else
            {
                auto ReplicatedOwner = InRequest.Get_Owner();
                const auto IsHost = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(ReplicatedOwner);
                ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] SpawnProcessor — Non-WithActor path, Owner=[{}] IsHost=[{}]"),
                    ReplicatedOwner, IsHost);

                if (IsHost)
                {
                    NewEntity.Add<ck::FRequest_EntityScript_Replicate>(
                        ReplicatedOwner, ContextOwnerOverride, InRequest.Get_SpawnParams(), NewEntityScript);
                }
                else
                {
                    ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] SpawnProcessor — SKIPPED replication request (owner is not host)"));
                }
            }
        }

        if (InRequest.Get_PostConstruction_Func())
        {
            InRequest.Get_PostConstruction_Func()(NewEntity);
        }

        Result = ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_SpawnEntity_CancelPendingRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment)
        -> void
    {
        InRequestFragment.TryFireCompletion(InRequestFragment.Get_NewEntity(),
            ECk_Request_OperationResult::Failed_Cancelled);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_ContinueConstruction::
        ForEachEntity(
            [[maybe_unused]]
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
            const TimeType&,
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

        ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] ReplicateProcessor — Handle=[{}] Owner=[{}] IsSelfOwned=[{}]"),
            InHandle, ReplicatedOwner, IsSelfOwned);

        // Defer (retry next frame, KEEP the dirty marker) until the ReplicationDriver exists: WithActor entities
        // acquire theirs via OwningActor::Add slightly after this request lands.
        if (NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
        {
            ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] ReplicateProcessor — Handle [{}] does NOT have ReplicationDriver yet, deferring"), InHandle);
            return;
        }

        // NOTE: Copy on purpose otherwise the data doesn't make its way over correctly
        const auto SpawnParamsCopy = InRequest.Get_SpawnParams();

        ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] ReplicateProcessor — Calling Request_ReplicateEntityScript for [{}] with owner [{}]"), InHandle, ReplicatedOwner);

        UCk_Utils_EntityScript_UE::Request_ReplicateEntityScript(InHandle, ReplicatedOwner, EntityScript.Get(), SpawnParamsCopy,
            InRequest.Get_ContextOwnerOverride());

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_FinishConstruction::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InHandle,
            TEXT("Construction finished, ready for BeginPlay"));
        InHandle.Add<FTag_EntityScript_BeginPlay>();

        // Mark "composed this frame" so the FGroup_DeferredApply dispatchers defer their apply until this frame's
        // pump / next frame — the feature Setups this construction just queued have not drained yet.
        InHandle.AddOrGet<FTag_EntityScript_ConstructedThisFrame>();

        UUtils_Signal_OnConstructed::Broadcast(InHandle, ck::MakePayload(InHandle));

        if (ck::Is_NOT_Valid(InCurrent.Get_Script()))
        { return; }

        auto WasConsumed = false;
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (ck::IsValid(LifetimeOwner) && LifetimeOwner.Has<FFragment_PendingReplication>())
        {
            auto& PendingFragment = LifetimeOwner.AddOrGet<FFragment_PendingReplication>();
            auto* Cdo = InCurrent.Get_Script()->GetClass()->GetDefaultObject<UCk_EntityScript_UE>();
            auto PendingEntity = PendingFragment.ConsumeFirst(
                InCurrent.Get_Script()->GetClass(), Cdo, InHandle);

            if (ck::IsValid(PendingEntity))
            {
                UUtils_Signal_OnConstructed::Broadcast(PendingEntity, ck::MakePayload(InHandle));
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntity);
                WasConsumed = true;
            }
        }

        if (NOT WasConsumed
            && InCurrent.Get_Script()->Get_EffectiveReplication() == ECk_Replication::Replicates
            && ck::IsValid(LifetimeOwner)
            && UCk_Utils_Net_UE::Get_EntityNetMode(LifetimeOwner) == ECk_Net_NetModeType::Client)
        {
            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
            const auto CurrentTime = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();

            InHandle.Add<FTag_EntityScript_PendingReplicationRetry>();
            InHandle.Add<FFragment_EntityScript_PendingReplicationRetryTimestamp>(CurrentTime);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

#if UE_BUILD_SHIPPING
    constexpr auto PendingReplicationRetryTimeoutSeconds = 2.0;
#else
    constexpr auto PendingReplicationRetryTimeoutSeconds = 10.0;
#endif

    auto
        FProcessor_EntityScript_PendingReplicationRetry::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent,
            const FFragment_EntityScript_PendingReplicationRetryTimestamp& InTimestamp)
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (ck::IsValid(LifetimeOwner) && LifetimeOwner.Has<FFragment_PendingReplication>())
        {
            auto& PendingFragment = LifetimeOwner.AddOrGet<FFragment_PendingReplication>();
            auto* Cdo = InCurrent.Get_Script()->GetClass()->GetDefaultObject<UCk_EntityScript_UE>();
            auto PendingEntity = PendingFragment.ConsumeFirst(
                InCurrent.Get_Script()->GetClass(), Cdo, InHandle);

            if (ck::IsValid(PendingEntity))
            {
                UUtils_Signal_OnConstructed::Broadcast(PendingEntity, ck::MakePayload(InHandle));
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntity);

                InHandle.Remove<FTag_EntityScript_PendingReplicationRetry>();
                InHandle.Remove<FFragment_EntityScript_PendingReplicationRetryTimestamp>();
                return;
            }
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto CurrentTime = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
        const auto ElapsedSeconds = (CurrentTime - InTimestamp.Get_TaggedAt()).Get_Seconds();

        if (ElapsedSeconds >= PendingReplicationRetryTimeoutSeconds)
        {
            ck::ecs::Warning(
                TEXT("PendingReplicationRetry timed out after [{}]s for entity [{}] with script [{}]. "
                     "No matching PendingEntity was found on the client."),
                ElapsedSeconds,
                InHandle,
                InCurrent.Get_Script()->GetClass()->GetName());

            InHandle.Remove<FTag_EntityScript_PendingReplicationRetry>();
            InHandle.Remove<FFragment_EntityScript_PendingReplicationRetryTimestamp>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_BeginPlay::
        ForEachEntity(
            const TimeType&,
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
            const TimeType&,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        auto* EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke EndPlay on it"), InHandle)
        { return; }

        CK_CALLSTACK_RECORD(ck::FFragment_EntityScript_Current, InHandle);
        EntityScript->EndPlay();
        InHandle.Add<FTag_EntityScript_HasEndedPlay>();

        // release the instance (Poolable recycles, force-new unpins; no-op for CDO/snapshot scripts).
        // Clear the weak ptr so a same-frame re-issue is unobservable through this dead entity
        UCk_Utils_Object_UE::TryReleaseToPool(EntityScript);
        InCurrent._Script.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------

