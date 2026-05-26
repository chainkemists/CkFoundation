#include "CkEntityScript_Processor.h"

#include "CkEntityScript.h"
#include "CkEntityScript_Utils.h"
#include "CkGenericEntityScript.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/DependencyInjection/CkDependencyProvider_GameInstance_Subsystem.h"
#include "CkEcs/DependencyInjection/CkDependencyProvider_World_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_SpawnEntity_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_ContinueConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_FinishConstruction);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_PendingReplicationRetry);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_BeginPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_AwaitingDependencies_Deadline);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityScript_FinishDeferredConstruct);
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
            NewEntity, NewEntityScript->Get_EffectiveReplication());

        // ---- Dependency Injection pass ----------------------------------------------------
        // Runs AFTER spawn-param injection so caller-supplied handles win (the DI pass only
        // fills in invalid fields). If any CkInject site cannot be resolved against a
        // registered provider, the entity is tagged AwaitingDependencies and Construct is
        // deferred — the resolution-callback path (provider lands → callback fires) or the
        // deadline processor (timeout → ensure + destroy) finishes the job.
        const auto AllResolved =
            UCk_Utils_EntityScript_UE::TryInjectEntityScriptDependencies(NewEntityScript, NewEntity);

        if (NOT AllResolved)
        {
            const auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(NewEntity);
            const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
            const auto Now = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();

            NewEntity.Add<FTag_EntityScript_AwaitingDependencies>();
            NewEntity.Add<FFragment_EntityScript_AwaitingDependencies>(
                NewEntityScript,
                Now,
                InRequest.Get_Owner(),
                InRequest.Get_SpawnParams(),
                InRequest.Get_PostConstruction_Func());

            CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
                TEXT("Construct deferred — awaiting CkInject dependencies"));

            return;
        }

        // ---- Shared finish-construction path ----------------------------------------------
        // Net_Params ensure, replication driver TryAdd, Construct switch, post-construct
        // replication block, and the PostConstruction callback all live in the shared helper
        // so the FinishDeferredConstruct processor (which runs after deferred resolution)
        // goes through exactly the same code path.
        UCk_Utils_EntityScript_UE::DoFinishConstructionFlow(
            NewEntityScript,
            NewEntity,
            InRequest.Get_Owner(),
            InRequest.Get_SpawnParams(),
            InRequest.Get_PostConstruction_Func());
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
            const TimeType&,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InHandle,
            TEXT("Construction finished, ready for BeginPlay"));
        InHandle.Add<FTag_EntityScript_BeginPlay>();

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
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke EndPlay on it"), InHandle)
        { return; }

        // Drop any DI pending entries this script registered. Prevents the
        // pending-bucket map from accumulating dead-script references over a
        // long session. Safe to call unconditionally — the subsystem walk
        // is keyed on script pointer and no-ops when there are no entries.
        if (auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            ck::IsValid(World))
        {
            if (auto* WorldSubsystem = World->GetSubsystem<UCk_DependencyProvider_World_Subsystem_UE>())
            { WorldSubsystem->UnregisterPending(EntityScript); }

            if (auto* GameInstance = World->GetGameInstance())
            {
                if (auto* GameInstanceSubsystem = GameInstance->GetSubsystem<UCk_DependencyProvider_GameInstance_Subsystem_UE>())
                { GameInstanceSubsystem->UnregisterPending(EntityScript); }
            }
        }

        CK_CALLSTACK_RECORD(ck::FFragment_EntityScript_Current, InHandle);
        EntityScript->EndPlay();
        InHandle.Add<FTag_EntityScript_HasEndedPlay>();
    }

    // --------------------------------------------------------------------------------------------------------------------
    //                            Dependency-Injection lifecycle processors
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_AwaitingDependencies_Deadline::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptCurrent,
            const FFragment_EntityScript_AwaitingDependencies& InAwaitingCurrent)
        -> void
    {
        auto* Script = InAwaitingCurrent.Get_Script().Get();
        if (Script == nullptr)
        {
            // Script gone — clean up and bail. Should not happen normally
            // (the StrongObjectPtr on the fragment keeps it alive), but
            // defensive in case the entity outlives its world.
            InHandle.Remove<FTag_EntityScript_AwaitingDependencies>();
            InHandle.Remove<FFragment_EntityScript_AwaitingDependencies>();
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(TimeParams);
        const auto Now        = TimeResult.Get_WorldTime().Get_Time();

        const auto Timeout = UCk_Utils_Ecs_Settings_UE::Get_DependencyInjection_Timeout();
        const auto Elapsed = Now - InAwaitingCurrent.Get_StartedAt();
        if (Elapsed.Get_Seconds() < Timeout.Get_Seconds())
        { return; }

        // Timeout — ensure and clean up. The script never made it past
        // Construct because at least one CkInject dependency never landed.
        CK_ENSURE_IF_NOT(false,
            TEXT("DependencyInjection timeout: EntityScript [{}] on entity [{}] waited [{}] "
                 "for a registered provider that never arrived. Check your "
                 "UCk_Utils_DependencyProvider_UE::Register calls. Destroying entity."),
            Script, InHandle, Timeout) {}

        // Drop pending callbacks from both subsystems so they don't fire
        // on a dead script if the provider lands later in the same world.
        if (auto* WorldSubsystem = World->GetSubsystem<UCk_DependencyProvider_World_Subsystem_UE>())
        { WorldSubsystem->UnregisterPending(Script); }

        if (auto* GameInstance = World->GetGameInstance())
        {
            if (auto* GameInstanceSubsystem = GameInstance->GetSubsystem<UCk_DependencyProvider_GameInstance_Subsystem_UE>())
            { GameInstanceSubsystem->UnregisterPending(Script); }
        }

        InHandle.Remove<FTag_EntityScript_AwaitingDependencies>();
        InHandle.Remove<FFragment_EntityScript_AwaitingDependencies>();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_FinishDeferredConstruct::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const FRequest_EntityScript_FinishDeferredConstruct& InRequest)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        auto* Script = InRequest.Get_Script().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("FinishDeferredConstruct fired with null script on entity [{}]"), InHandle)
        { return; }

        auto Entity = InHandle.ConvertToHandle();
        UCk_Utils_EntityScript_UE::DoFinishConstructionFlow(
            Script, Entity,
            InRequest.Get_LifetimeOwner(),
            InRequest.Get_OriginalSpawnParams(),
            InRequest.Get_PostConstruction_Func());
    }
}

// --------------------------------------------------------------------------------------------------------------------

