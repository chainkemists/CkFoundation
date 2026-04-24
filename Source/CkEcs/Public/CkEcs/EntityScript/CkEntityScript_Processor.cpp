#include "CkEntityScript_Processor.h"

#include "CkEntityScript.h"
#include "CkEntityScript_Utils.h"
#include "CkGenericEntityScript.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_SpawnEntity_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_ContinueConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_FinishConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_PendingReplicationRetry);
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
        // Gate child spawns on the parent being fully constructed. ContinueConstruction and
        // FinishConstruction are true "construction in progress" markers — a child EntityScript
        // spawned from inside a parent's Construct callback must wait for the parent to finish
        // its own pipeline before we create it.
        if (const auto& LifetimeOwner = InRequestFragment.Get_Owner();
            LifetimeOwner.Has_Any<FTag_EntityScript_ContinueConstruction, FTag_EntityScript_FinishConstruction>())
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

        ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — Entity=[{}] Replication=[{}]"),
            NewEntity, NewEntityScript->Get_Replication());

        // ---- Net Params (before Construct) ------------------------------------------------
        // Replication intent is script-authoritative: always re-stamp _Replication from the
        // script's Get_EffectiveReplication, overriding any value inherited from the lifetime
        // owner. NetMode / NetRole come from the inherited fragment when present, otherwise
        // from the TransientEntity's pre-computed settings (set by the Net Subsystem from the
        // World). On clients, entities going through this spawn path are proxies.
        {
            const auto EffectiveReplication = NewEntityScript->Get_EffectiveReplication(InRequest.Get_SpawnParams());

            if (NewEntity.Has<ck::FFragment_Net_Params>())
            {
                NewEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Set_Replication(EffectiveReplication);
            }
            else
            {
                const auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(NewEntity);
                const auto& Settings = TransientEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings();

                auto NetRole = Settings.Get_NetRole();

                if (EffectiveReplication == ECk_Replication::Replicates
                    && Settings.Get_NetMode() == ECk_Net_NetModeType::Client)
                {
                    NetRole = ECk_Net_EntityNetRole::Proxy;
                }

                UCk_Utils_Net_UE::Add(NewEntity, FCk_Net_ConnectionSettings{
                    EffectiveReplication, Settings.Get_NetMode(), NetRole});
            }
        }

        CK_ENSURE_IF_NOT(NewEntity.Has<ck::FFragment_Net_Params>(),
            TEXT("Entity [{}] is missing NetParams before construction. EntityScript: [{}]"),
            NewEntity, EntityScriptClassArchetype) {}

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
#if WITH_EDITOR
                const auto& IsBlueprintClass = ck::IsValid(NewEntityScript->GetClass()->ClassGeneratedBy);
#else
                // In non-editor builds, assume it's a Blueprint class
                const auto& IsBlueprintClass = true;
#endif
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_GenericEntityScript_UE, DoContinueConstruction);
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

        // ---- Replication Infrastructure (after Construct) ---------------------------------
        // OwningActor is now available (WithActor adds it during Construct). Set up the
        // ReplicationDriver, enable actor replication, and enqueue replication requests.
        if (UCk_Utils_Net_UE::Get_Replication(NewEntity) == ECk_Replication::Replicates)
        {
            const auto HasOwningActor = UCk_Utils_OwningActor_UE::Has(NewEntity);
            ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — HasOwningActor=[{}]"), HasOwningActor);

            if (HasOwningActor)
            {
                auto* OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(NewEntity);
                auto* World = OwningActor->GetWorld();

                const auto IsNetworkedAuthority =
                    World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);

                ck::ecs::Display(TEXT("[REP_DEBUG] SpawnProcessor — WithActor path, IsNetworkedAuthority=[{}]"),
                    IsNetworkedAuthority);

                // Only set up the ReplicationDriver on a networked authority. In
                // NM_Standalone there is no net driver, so the UCk_Fragment_EntityReplicationDriver_Rep
                // UObject would not be GC-pinned and its BeginDestroy would tear down
                // the entity. On NM_Client, authority-side replication is not our concern.
                if (IsNetworkedAuthority)
                {
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

        auto WasConsumed = false;

        if (ck::IsValid(InCurrent.Get_Script()))
        {
            auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

            if (ck::IsValid(LifetimeOwner) && LifetimeOwner.Has<FFragment_PendingReplication>())
            {
                auto& PendingFragment = LifetimeOwner.AddOrGet<FFragment_PendingReplication>();
                auto* CDO = InCurrent.Get_Script()->GetClass()->GetDefaultObject<UCk_EntityScript_UE>();
                auto PendingEntity = PendingFragment.ConsumeFirst(
                    InCurrent.Get_Script()->GetClass(), CDO, InHandle);

                if (ck::IsValid(PendingEntity))
                {
                    UUtils_Signal_OnConstructed::Broadcast(PendingEntity, ck::MakePayload(InHandle));
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PendingEntity);
                    WasConsumed = true;
                }
            }

            if (NOT WasConsumed
                && UCk_Utils_Net_UE::Get_Replication(InHandle) == ECk_Replication::Replicates
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
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent,
            const FFragment_EntityScript_PendingReplicationRetryTimestamp& InTimestamp)
        -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        if (ck::IsValid(LifetimeOwner) && LifetimeOwner.Has<FFragment_PendingReplication>())
        {
            auto& PendingFragment = LifetimeOwner.AddOrGet<FFragment_PendingReplication>();
            auto* CDO = InCurrent.Get_Script()->GetClass()->GetDefaultObject<UCk_EntityScript_UE>();
            auto PendingEntity = PendingFragment.ConsumeFirst(
                InCurrent.Get_Script()->GetClass(), CDO, InHandle);

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

