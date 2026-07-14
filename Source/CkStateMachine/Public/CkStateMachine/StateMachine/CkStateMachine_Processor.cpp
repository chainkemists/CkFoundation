#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/CkStateMachine_Stats.h"
#include "CkStateMachine/Net/CkStateMachineRelay_Actor.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/Net/CkStateMachine_TestSupport.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#if !UE_BUILD_SHIPPING
#include "CkCore/Object/CkObject_Utils.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_Utils.h"
#endif

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_FlushPendingReplication_Drain);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_FirstSyncInitialState);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_ApplyReplicatedHistory);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_CommitPendingTransition);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_HydrationResume);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_PushOwningClientBatch);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmScript_CommitPendingAttach);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Sm::Setup"), STAT_Sm_Setup, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::HandleRequests"), STAT_Sm_HandleRequests, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::CommitPendingTransition"), STAT_Sm_CommitPendingTransition, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::FlushPendingReplication_Drain"), STAT_Sm_FlushPendingReplication_Drain, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::ApplyReplicatedHistory"), STAT_Sm_ApplyReplicatedHistory, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::FirstSyncInitialState"), STAT_Sm_FirstSyncInitialState, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::HydrationResume"), STAT_Sm_HydrationResume, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::PushOwningClientBatch"), STAT_Sm_PushOwningClientBatch, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("Sm::EndPlay"), STAT_Sm_EndPlay, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmScript::CommitPendingAttach"), STAT_SmScript_CommitPendingAttach, STATGROUP_CkStateMachine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sm Transitions Committed"), STAT_SmTransitionsCommitted, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // SETUP
    // ================================================================================================================

    auto
        FProcessor_Sm_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_Setup);

        InHandle.Remove<FTag_Sm_RequiresSetup>();

        // Attach the replicated payload fragment on authority. Gated on both the per-SM
        // replication intent (so local-only SMs are unaffected) and Get_IsEntityNetMode_Host
        // (so clients don't try to drive the payload — they receive it via the rep handler).
        // TryAddContainerFragment is itself a no-op when entity replication is DoesNotReplicate
        // or no replication driver is present, but gating here makes the intent explicit and
        // avoids the driver lookup in the local-only fast path.
        if (InParams.Get_Replication() == ECk_Replication::Replicates
            && UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle))
        {
            switch (InParams.Get_ReplicationModel())
            {
                case ECk_Sm_ReplicationModel::WithHistory:
                {
                    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                        InHandle, FCk_RepData_StateMachine_WithHistory{});
                    break;
                }
                case ECk_Sm_ReplicationModel::WithoutHistory:
                {
                    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_StateMachine_NoHistory>(
                        InHandle, FCk_RepData_StateMachine_NoHistory{});
                    break;
                }
            }
        }

        if (InParams.Get_AutoStart() == ECk_SmAutoStart::OnSetup)
        {
            auto& Requests = InHandle.AddOrGet<FFragment_Sm_Requests>();
            Requests._Requests.Add(FCk_Request_Sm_Start{});
        }
    }

    // ================================================================================================================
    // HANDLE REQUESTS
    // ================================================================================================================

    auto
        FProcessor_Sm_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Requests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_HandleRequests);

        // Authority gating (spec §5.6/§6): mutating requests (Start/Stop/Pause/Resume/Transition/
        // AddOverrideState) may only originate on the authority for this SM. Non-authority
        // machines receive state changes via replication and bypass this processor entirely
        // through FProcessor_Sm_ApplyReplicatedHistory (Phase 7). DoesNotReplicate SMs are
        // self-authoritative on every machine — no gating needed.
        //
        // OwningClientAuthoritative authority = "the machine that locally controls the owning
        // actor". On a remote client that resolves to NetContext == OwningClient. On a LISTEN
        // SERVER, the host is both the server (ComputeNetContext short-circuits to Server before
        // the locally-controlled check) AND the owning client for its own pawn — so it is the
        // authority too. We detect that explicitly: Server context + OwningClientAuth + the host
        // locally controls this SM's owning actor. A dedicated server controls no player pawn, so
        // this stays false there (the remote owning client remains the sole authority), and it
        // stays false for the host's view of OTHER players' pawns.
        const auto NetContext    = ck::statemachine::ComputeNetContext(InHandle);
        const auto EffectiveAuth = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InHandle);
        const auto IsListenServerHostOwningClient =
            NetContext == ECk_Sm_NetContext::Server
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InHandle)
                == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;
        const auto IsRequestAuthority =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || NetContext == ECk_Sm_NetContext::Standalone
            || (NetContext == ECk_Sm_NetContext::Server
                && EffectiveAuth == ECk_Sm_AuthorityModel::ServerAuthoritative)
            || (NetContext == ECk_Sm_NetContext::OwningClient
                && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative)
            || IsListenServerHostOwningClient;

        if (NOT IsRequestAuthority)
        {
            // Every branch below drops the whole batch. Dropped requests may include Transitions
            // whose enqueue-side added FTag_Sm_TransitionQueued — clear it up front so the
            // evaluator isn't blocked forever.
            InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

            // AutoStart (FProcessor_Sm_Setup) enqueues a Start on EVERY machine, including
            // non-authority ones. On a non-authority copy of a Replicates SM that Start is
            // expected to be dropped here — the SM reaches Running via the replicated run-status
            // (server relay / rep mirror), not its local AutoStart — so it is NOT misuse. Only the
            // authority-only mutations (Stop/Pause/Resume/Transition/AddOverrideState), which can
            // only originate from an explicit Request_*, signal a programming error on a
            // non-authority machine. Scope the ensure to those; a pure-Start batch drops silently.
            const auto NonStartRequestCount = algo::CountIf(InRequests.Get_Requests(),
                [](const FFragment_Sm_Requests::RequestType& InRequest) -> bool
                {
                    return NOT std::holds_alternative<FCk_Request_Sm_Start>(InRequest);
                });

            CK_ENSURE_IF_NOT(NonStartRequestCount == 0,
                TEXT("Non-authority machine attempted to enqueue [{}] authority-only SM request(s) on [{}] "
                     "(NetContext [{}], AuthorityModel [{}]). Requests dropped. Rep-driven transitions "
                     "bypass this processor via the replay path."),
                NonStartRequestCount, InHandle, NetContext, EffectiveAuth)
            {
                InHandle.Try_Remove<FFragment_Sm_Requests>();
                return;
            }

            // Pure AutoStart echo on a non-authority machine — drop silently; the SM reaches
            // Running via the replicated run-status.
            InHandle.Try_Remove<FFragment_Sm_Requests>();
            return;
        }

        InHandle.CopyAndRemove(InRequests, [&](FFragment_Sm_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Helper — write the SM's current run-status into the replicated payload when the local
    // machine is the authority for this SM. Used by Start/Stop/Pause/Resume handlers to mirror
    // run-status across the wire. Local-only and non-authority machines silently skip
    // (TryUpdateContainerFragment is itself a no-op without an active rep driver, but checking
    // params explicitly keeps the intent visible).
    static auto
        DoPublishRunStatus(
            FCk_Handle_StateMachine& InSm,
            const FFragment_Sm_Params& InParams,
            ECk_SmRunStatus InNewStatus)
        -> void
    {
        if (InParams.Get_Replication() != ECk_Replication::Replicates)
        { return; }

        const auto NetContext = ck::statemachine::ComputeNetContext(InSm);
        const auto AuthModel  = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InSm);

        // Mirror the transition-publish split (FProcessor_Sm_CommitPendingTransition): the server
        // is the canonical rep publisher regardless of authority model. This is what carries the
        // listen-server host's OwningClientAuth run-status to other clients — the host commits
        // locally as the owning client (see the gating in ForEachEntity), then publishes here as
        // the server. A REMOTE owning client (NetContext == OwningClient) instead buffers for the
        // relay so the server can republish. A dedicated server reaches this only via the relay
        // handler, not here, since it never locally drives an owning-client SM's run-status.
        const auto IsRepPublisher = NetContext == ECk_Sm_NetContext::Server;

        const auto IsOwningClientOriginator =
            NetContext == ECk_Sm_NetContext::OwningClient
            && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative;

        if (NOT IsRepPublisher && NOT IsOwningClientOriginator)
        { return; }

        // Owning-client run-status changes can't be published through the server→client rep
        // container (the client doesn't own it — TryUpdateContainerFragment would no-op). Buffer
        // the run-status for FProcessor_Sm_PushOwningClientBatch to relay via Server_PushRunStatus,
        // mirroring how owning-client transitions buffer for Server_PushTransitionBatch. Without
        // this the server's OwningClientAuth SM never reaches Running and ApplyReplicatedHistory
        // drops every relayed transition.
        if (IsOwningClientOriginator)
        {
            auto& Batch = InSm.AddOrGet<FFragment_Sm_PendingClientBatch>();
            Batch.Set_PendingRunStatus(InNewStatus);
            Batch.Set_HasPendingRunStatus(true);
            return;
        }

        // IsRepPublisher (server, including the listen-server host): write the rep container so
        // non-owning clients pick up the change.
        switch (InParams.Get_ReplicationModel())
        {
            case ECk_Sm_ReplicationModel::WithHistory:
            {
                UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                    InSm,
                    [&](FCk_RepData_StateMachine_WithHistory& RepData) -> void
                    {
                        RepData.Set_RunStatus(InNewStatus);
                    });
                break;
            }
            case ECk_Sm_ReplicationModel::WithoutHistory:
            {
                UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_NoHistory>(
                    InSm,
                    [&](FCk_RepData_StateMachine_NoHistory& RepData) -> void
                    {
                        RepData.Set_RunStatus(InNewStatus);
                    });
                break;
            }
        }
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Stopped)
        {
            // Paused is rejected too, not just Running: falling through would Add a duplicate
            // FTag_Sm_Running (ensure) and DoEnterState would stack a fresh initial state on top
            // of the still-alive paused current state. Resume is the way back from Paused.
            if (InCurrent._RunStatus == ECk_SmRunStatus::Paused)
            {
                ck::sm::Warning(
                    TEXT("Request_Start on [{}] while Paused — ignored. Use Request_Resume."), InHandle);
            }
            return;
        }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Add<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Running);

        DoEnterState(InHandle, InCurrent, InParams.Get_InitialStateClass());

        UUtils_Signal_OnSmStarted::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStarted{}));

        // Broadcast OnSmStateChanged for the initial state entry too —
        // PreviousStateClass=null signals "no prior state; this is the SM's
        // first state." Without this fire, a sink-state (zero-transition)
        // SM never produces OnSmStateChanged at all, and consumers binding
        // to react per-state-entry miss the initial entry forever.
        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                TSubclassOf<UCk_SmState_EntityScript>{},
                InCurrent._CurrentStateClass,
                InCurrent._CurrentStateHandle
            }));

        UCk_Utils_StateMachine_UE::TryCheckEntryBreakpoint(InHandle, InParams.Get_InitialStateClass());

#if !UE_BUILD_SHIPPING
        // If this SM has an owning SM, it's a sub-SM — surface its initial-state entry in the
        // parent SM's history. Otherwise a sub-SM that enters its initial state and dies in the
        // same frame (e.g. an entry task that immediately finishes the sub-SM) leaves no trace
        // in the debugger because sub-SM history is only synthesized from transitions, and its
        // own fragment is destroyed before the polling debug processor can snapshot it.
        if (TUtils_Sm_OwningStateMachine::Has(InHandle))
        {
            auto ParentSm = TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);

            auto ParentStateName = FString{};
            if (ck::IsValid(ParentSm) && ParentSm.Has<FFragment_Sm_Current>())
            {
                if (const auto ParentCurrentClass = ParentSm.Get<FFragment_Sm_Current>().Get_CurrentStateClass();
                    ck::IsValid(ParentCurrentClass))
                {
                    ParentStateName = UCk_Utils_Object_UE::Get_CleanClassName(ParentCurrentClass);
                }
            }

            // Use _CurrentStateClass (already override-resolved by DoEnterState above) rather
            // than InParams.Get_InitialStateClass(), which is the pre-resolution request.
            // Otherwise overridden sub-SMs would display the base class name instead of the
            // actual running class.
            auto SubSmStartRequest = FCk_Request_SmDebug_RecordTransition{
                TSubclassOf<UCk_SmState_EntityScript>{}, InCurrent._CurrentStateClass};
            SubSmStartRequest.Set_FrameNumber(UCk_Utils_Time_UE::Get_FrameNumber());
            SubSmStartRequest.Set_RealTimeSeconds(FPlatformTime::Seconds());
            SubSmStartRequest.Set_SubSmParentStateName(ParentStateName);

            UCk_Utils_StateMachineDebug_UE::Request_RecordTransition(ParentSm, SubSmStartRequest);
        }
#endif
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest)
        -> void
    {
        if (InCurrent._RunStatus == ECk_SmRunStatus::Stopped)
        { return; }

        DoExitCurrentState(InHandle, InCurrent);

        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InHandle.Try_Remove<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();
        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

        // A transition can be mid-flight (exit cascade ran, commit hasn't) — discard it here, or
        // its kept-alive previous state leaks: HandleRequests runs before CommitPendingTransition,
        // so the commit's own not-Running cleanup never sees the fragment once Stop removes it.
        UCk_Utils_StateMachine_UE::Request_TryDiscardPendingTransition(InHandle);

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Stopped);

        UUtils_Signal_OnSmStopped::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStopped{}));
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Paused;
        InHandle.Add<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Paused);
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Paused)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Remove<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Running);
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest)
        -> void
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        {
            // The enqueue side unconditionally added FTag_Sm_TransitionQueued; a dropped request
            // must clear it or FProcessor_SmState_Evaluate blocks forever (Pause →
            // Request_Transition → Resume left the SM permanently unable to evaluate).
            InHandle.Try_Remove<FTag_Sm_TransitionQueued>();
            return;
        }

        const auto PreviousStateClass  = InCurrent._CurrentStateClass;
        const auto PreviousStateHandle = InCurrent._CurrentStateHandle;

        UCk_Utils_StateMachine_UE::TryCheckExitBreakpoint(InHandle, PreviousStateClass);

        // Run the exit cascade but DO NOT destroy the previous state yet — its handle is stashed
        // into FFragment_Sm_PendingTransition below and read by FProcessor_Sm_CommitPendingTransition
        // (Get_IsPendingExit). Destroying it here (deferred to end-of-frame) can win the race
        // against the commit when the exit cascade straddles a frame boundary, leaving the commit
        // to query a tombstone handle. CommitPendingTransition destroys it after the new state is
        // entered.
        constexpr auto ScheduleDestroyNow = false;
        DoExitCurrentState(InHandle, InCurrent, ScheduleDestroyNow);

        // The actual entry happens in FProcessor_Sm_CommitPendingTransition, after the
        // exit cascade (state -> task -> transition -> condition) has fully drained.
        //
        // A second Request_Transition can land before the first commits (same drain batch, or an
        // exit cascade straddling frames). Coalesce: keep the ORIGINAL previous-state stash — it
        // holds the only reference to the kept-alive exited state (overwriting it leaked the
        // entity and published a null PreviousStateClass) — and only retarget. The never-entered
        // intermediate target is skipped entirely (observers see one A→C transition).
        const auto HadPendingTransition = InHandle.Has<FFragment_Sm_PendingTransition>();
        auto& Pending = InHandle.AddOrGet<FFragment_Sm_PendingTransition>();
        if (NOT HadPendingTransition)
        {
            Pending._PreviousStateHandle = PreviousStateHandle;
            Pending._PreviousStateClass  = PreviousStateClass;
        }
        Pending._TargetStateClass = InRequest.Get_TargetStateClass();

        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_AddOverrideState& InRequest)
        -> void
    {
        const auto& OverrideClass = InRequest.Get_OverrideStateClass();

        CK_ENSURE_IF_NOT(ck::IsValid(OverrideClass),
            TEXT("FCk_Request_Sm_AddOverrideState on [{}] has invalid override state class"), InHandle)
        { return; }

        auto* CDO = OverrideClass->GetDefaultObject<UCk_SmState_EntityScript>();
        const auto StatesToOverride = CDO->Get_StatesToOverride();

        CK_ENSURE_IF_NOT(StatesToOverride.Num() > 0,
            TEXT("FCk_Request_Sm_AddOverrideState on [{}]: override class [{}] returns empty Get_StatesToOverride()"),
            InHandle, *OverrideClass->GetName())
        { return; }

        auto& Overrides = InHandle.AddOrGet<FFragment_Sm_StateOverrides>();
        Overrides._Overrides.Add(FFragment_Sm_StateOverrides::FEntry{OverrideClass, StatesToOverride});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_HandleRequests::
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InStateClass),
            TEXT("Invalid state class when entering state on SM [{}]"), InSmHandle)
        { return; }

        InCurrent._CurrentStateHandle = UCk_Utils_SmState_UE::Create(InSmHandle, InStateClass);
        InCurrent._CurrentStateClass = UCk_Utils_SmState_UE::Get_ResolvedStateClass(InSmHandle, InStateClass);
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            bool InScheduleDestroy)
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent._CurrentStateHandle))
        {
            ck::sm::VeryVerbose(TEXT("[SM Lifecycle] DoExitCurrentState on SM [{}] — no current state, nothing to exit"),
                InSmHandle);
            return;
        }

        ck::sm::VeryVerbose(TEXT("[SM Lifecycle] DoExitCurrentState on SM [{}] -> exiting state [{}]"),
            InSmHandle, InCurrent._CurrentStateHandle);

        UCk_Utils_SmState_UE::Request_Exit(InCurrent._CurrentStateHandle, InScheduleDestroy);

        InCurrent._CurrentStateHandle = FCk_Handle_SmState{};
        InCurrent._CurrentStateClass = nullptr;
    }

    // ================================================================================================================
    // COMMIT PENDING TRANSITION
    // ================================================================================================================

    auto
        FProcessor_Sm_CommitPendingTransition::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_PendingTransition& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_CommitPendingTransition);

        // The transition path keeps the previous state ALIVE until here (DoExitCurrentState was
        // called with ScheduleDestroyNow=false), so its handle is still valid and we destroy it
        // below, after the new state is entered. We still wait while it is mid-exit
        // (FProcessor_SmState_Exit hasn't cleared FTag_SmState_PendingExit yet). The validity
        // guard covers the initial transition (no previous state) and any caller that released it.
        if (ck::IsValid(InPending._PreviousStateHandle)
            && UCk_Utils_SmState_UE::Get_IsPendingExit(InPending._PreviousStateHandle))
        { return; }

        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        {
            // Discard rather than just remove — the deferred-alive previous state must not leak
            // when the SM stopped mid-transition.
            UCk_Utils_StateMachine_UE::Request_TryDiscardPendingTransition(InHandle);
            return;
        }

        const auto PreviousStateClass     = InPending._PreviousStateClass;
        const auto TargetStateClass       = InPending._TargetStateClass;
        const auto IncomingNewFingerprint = InPending._NewStateFingerprint;
        const auto PreviousStateHandle    = InPending._PreviousStateHandle;

        FProcessor_Sm_HandleRequests::DoEnterState(InHandle, InCurrent, TargetStateClass);

        // New state is now committed — it is safe to destroy the previous state entity that the
        // transition path deliberately kept alive (see DoHandleRequest(Transition)). Its exit
        // lifecycle already ran via FProcessor_SmState_Exit; EndPlay's ExitState is a no-op
        // (FTag_SmState_Active dedup).
        if (ck::IsValid(PreviousStateHandle))
        {
            auto PreviousToDestroy = FCk_Handle{PreviousStateHandle};
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PreviousToDestroy);
        }

        // Pass the replicated fingerprint expectation to the new state entity so that its
        // Construct/DoComputeFingerprint cycle can verify against it (spec §9). Authority-
        // driven commits (server's own transitions, owning client's own) leave
        // _NewStateFingerprint at 0 — no carrier, no verify on the local instance.
        // The fragment lives just long enough for Construct's deferred verify pass.
        if (IncomingNewFingerprint != 0 && ck::IsValid(InCurrent._CurrentStateHandle))
        {
            auto& Expected = InCurrent._CurrentStateHandle.AddOrGet<FFragment_SmState_ExpectedFingerprint>();
            Expected.Set_Hash(IncomingNewFingerprint);
        }

#if !UE_BUILD_SHIPPING
        if (ck::IsValid(PreviousStateClass) && ck::IsValid(InCurrent._CurrentStateClass))
        {
            auto Request = FCk_Request_SmDebug_RecordTransition{
                PreviousStateClass, InCurrent._CurrentStateClass};
            Request.Set_FrameNumber(UCk_Utils_Time_UE::Get_FrameNumber());

            if (InHandle.Has<FFragment_Sm_Debug_LastFiredTransition>())
            {
                const auto& LastFired = InHandle.Get<FFragment_Sm_Debug_LastFiredTransition>();
                Request.Set_ConditionNames(LastFired.ConditionNames);
                Request.Set_RealTimeSeconds(LastFired.RealTimeSeconds);
                InHandle.Remove<FFragment_Sm_Debug_LastFiredTransition>();
            }
            else
            {
                Request.Set_RealTimeSeconds(FPlatformTime::Seconds());
            }

            UCk_Utils_StateMachineDebug_UE::Request_RecordTransition(InHandle, Request);
        }
#endif

        INC_DWORD_STAT(STAT_SmTransitionsCommitted);

        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                PreviousStateClass,
                TargetStateClass,
                InCurrent._CurrentStateHandle
            }));

        UCk_Utils_StateMachine_UE::TryCheckEntryBreakpoint(InHandle, TargetStateClass);

        // Replication publication and client-batch buffering. Two disjoint paths:
        //
        // 1. IsRepPublisher: NetContext == Server. The server is the canonical publisher for
        //    Replicates SMs regardless of authority model. For ServerAuth SMs the server is
        //    both originator and publisher; for OwningClientAuth SMs the server commits via
        //    RPC and republishes via the same write path. Non-owning clients receive RepData
        //    deltas from this write and replay through ApplyReplicatedHistory.
        //
        // 2. IsOwningClientOriginator: NetContext == OwningClient && AuthorityModel ==
        //    OwningClientAuthoritative. The owning client commits locally for zero latency
        //    and buffers the event into FFragment_Sm_PendingClientBatch for the
        //    FProcessor_Sm_PushOwningClientBatch processor to flush via RPC. The server's
        //    handler (Server_PushTransitionBatch) replays into the server's own pipeline,
        //    eventually hitting branch (1) to broadcast to other clients.
        //
        // Non-owning clients (NetContext == NonOwningClient) hit neither branch — they don't
        // publish or buffer, they only consume rep deltas from OnChange/OnAdd.
        if (InParams.Get_Replication() == ECk_Replication::Replicates)
        {
            const auto NetContext = ck::statemachine::ComputeNetContext(InHandle);
            const auto AuthModel  = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InHandle);

            const auto IsRepPublisher          = NetContext == ECk_Sm_NetContext::Server;
            const auto IsOwningClientOriginator =
                NetContext == ECk_Sm_NetContext::OwningClient
                && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative;

            if (IsRepPublisher || IsOwningClientOriginator)
            {
                auto& NextSeq = InHandle.AddOrGet<FFragment_Sm_NextSeq>();
                const auto SeqValue = NextSeq.Get_Next();
                NextSeq.Set_Next(SeqValue + 1);

                // Per-class fingerprint cache lookup. Populated by UCk_SmState_EntityScript::
                // DoComputeFingerprint after every state's first Construct anywhere — server,
                // client, any SM instance. After warm-up the cache returns the real hash here
                // and clients can verify on commit. On the very first instantiation of a class
                // anywhere in this process, the lookup returns 0; the Construct backfill path
                // (DoBackfillFingerprintToRepData) cleans up that single zero asynchronously.
                auto NewStateFingerprint =
                    UCk_SmState_EntityScript::Get_CachedFingerprint(TargetStateClass);

#if WITH_DEV_AUTOMATION_TESTS
                // Test-only fake-fingerprint injection (spec §13 tests 8 + 9). When armed via
                // UCk_Utils_StateMachine_Test_UE::Test_InjectFakeFingerprint, replace the cached
                // value with the test-supplied fake before it reaches the wire. Receive side
                // runs the genuine verify path; tests assert the determinism fault response.
                if (InHandle.Has<FFragment_Sm_TestFakeFingerprintInjection>())
                {
                    auto& Injection = InHandle.Get<FFragment_Sm_TestFakeFingerprintInjection>();
                    if (Injection.Get_Scope() == ECk_Sm_TestFakeFingerprintScope::NextTransition)
                    {
                        NewStateFingerprint = Injection.Get_FakeFingerprint();
                        if (Injection.Get_ConsumeOnUse())
                        { InHandle.Try_Remove<FFragment_Sm_TestFakeFingerprintInjection>(); }
                    }
                }
#endif

                const auto Event = FCk_Sm_TransitionEvent
                {
                    PreviousStateClass,
                    TargetStateClass,
                    SeqValue,
                    NewStateFingerprint
                };

                if (IsRepPublisher)
                {
                    switch (InParams.Get_ReplicationModel())
                    {
                        case ECk_Sm_ReplicationModel::WithHistory:
                        {
                            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                                InHandle,
                                [&](FCk_RepData_StateMachine_WithHistory& RepData) -> void
                                {
                                    auto& History = RepData.Get_History();
                                    History.Add(Event);
                                    if (History.Num() > FCk_RepData_StateMachine_WithHistory::RingSize)
                                    { History.RemoveAt(0); }
                                });
                            break;
                        }
                        case ECk_Sm_ReplicationModel::WithoutHistory:
                        {
                            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_NoHistory>(
                                InHandle,
                                [&](FCk_RepData_StateMachine_NoHistory& RepData) -> void
                                {
                                    RepData.Set_CurrentStateClass(TargetStateClass);
                                    RepData.Set_Seq(SeqValue);
                                    RepData.Set_CurrentStateFingerprint(NewStateFingerprint);
                                });
                            break;
                        }
                    }
                }
                else // IsOwningClientOriginator — buffer for FProcessor_Sm_PushOwningClientBatch
                {
                    auto& Batch = InHandle.AddOrGet<FFragment_Sm_PendingClientBatch>();
                    Batch.Get_PendingEvents().Add(Event);
                }
            }
        }

        // Sub-SM transition relay. A sub-SM created by UCk_SmTask_SubStateMachine is DoesNotReplicate
        // (no transport of its own), so the Replicates publish block above is skipped for it entirely.
        // On the OWNING CLIENT of a replicated OwningClientAuthoritative root, route the sub-SM's
        // transition through the ROOT's client->server batch (the root rides the pawn entity's
        // replication driver), tagged with the sub-SM's parent-hierarchy identity so the server /
        // non-owning clients dispatch it to the matching local sub-SM. The server & non-owning copies
        // of the sub-SM commit via that relayed replay — they are not the owning client, so they don't
        // re-originate here.
        if (InParams.Get_Replication() != ECk_Replication::Replicates)
        {
            const auto SubSmIdentity = UCk_Utils_StateMachine_UE::Get_SubSmParentHierarchy(InHandle);
            if (NOT SubSmIdentity.IsEmpty())
            {
                const auto NetContext = ck::statemachine::ComputeNetContext(InHandle);
                const auto AuthModel  = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InHandle);
                const auto IsOwningClientOriginator =
                    NetContext == ECk_Sm_NetContext::OwningClient
                    && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative;
                // Leg 2 (server -> non-owning clients): the server commits this sub-SM transition only
                // via the relayed replay queue (it is not the transition authority for an OwningClientAuth
                // sub-SM — see Get_IsTransitionAuthority), so every sub-SM transition it commits is a
                // relayed one that must be republished to observers.
                const auto IsServerRepublisher =
                    NetContext == ECk_Sm_NetContext::Server
                    && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative;

                if (IsOwningClientOriginator || IsServerRepublisher)
                {
                    auto Root = UCk_Utils_StateMachine_UE::Get_RootStateMachine(InHandle);
                    if (UCk_Utils_StateMachine_UE::Get_Replication(Root) == ECk_Replication::Replicates)
                    {
                        auto& NextSeq = InHandle.AddOrGet<FFragment_Sm_NextSeq>();
                        const auto SeqValue = NextSeq.Get_Next();
                        NextSeq.Set_Next(SeqValue + 1);

                        auto Event = FCk_Sm_TransitionEvent
                        {
                            PreviousStateClass,
                            TargetStateClass,
                            SeqValue,
                            UCk_SmState_EntityScript::Get_CachedFingerprint(TargetStateClass)
                        };
                        Event.Set_SubSmIdentity(SubSmIdentity);

                        if (IsOwningClientOriginator)
                        {
                            // Leg 1 (owning client -> server): buffer onto the root's client batch for
                            // FProcessor_Sm_PushOwningClientBatch to RPC via Server_PushTransitionBatch.
                            auto& Batch = Root.AddOrGet<FFragment_Sm_PendingClientBatch>();
                            Batch.Get_PendingEvents().Add(Event);
                        }
                        else // IsServerRepublisher
                        {
                            // Republish the identity-tagged event onto the ROOT's WithHistory container
                            // (the root rides the pawn entity's replication driver) so non-owning observers
                            // dispatch it to their local sub-SM by identity. The owning client echo-
                            // suppresses this delta (it already applied the transition locally). NoHistory
                            // roots have no field to carry the identity, so the relay is WithHistory-only —
                            // mirrors leg 1's Server_PushTransitionBatch (WithHistory) vs Server_PushCurrentState.
                            if (UCk_Utils_StateMachine_UE::Get_ReplicationModel(Root) == ECk_Sm_ReplicationModel::WithHistory)
                            {
                                UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                                    Root,
                                    [&](FCk_RepData_StateMachine_WithHistory& RepData) -> void
                                    {
                                        auto& History = RepData.Get_History();
                                        History.Add(Event);
                                        if (History.Num() > FCk_RepData_StateMachine_WithHistory::RingSize)
                                        { History.RemoveAt(0); }
                                    });

                                ck::sm::Verbose(TEXT("[SubSmRelay] server republished sub-SM transition [{}] -> [{}] (seq [{}]) onto root [{}] WithHistory container"),
                                    PreviousStateClass, TargetStateClass, SeqValue, Root);
                            }
                        }
                    }
                }
            }
        }

        InHandle.Try_Remove<FFragment_Sm_PendingTransition>();

        // Apply a parked run-status mirror once the replay pipeline is drained — preserves the
        // authority's transitions-then-status ordering. See FFragment_Sm_DeferredRunStatusMirror
        // for why Paused/Stopped mirrors must not jump ahead of queued replayed transitions.
        // (The pending-transition fragment was removed just above, so in-flight here means
        // "replay queue still holds events".)
        if (InHandle.Has<FFragment_Sm_DeferredRunStatusMirror>()
            && NOT UCk_Utils_StateMachine_UE::Get_HasReplayTransitionsInFlight(InHandle))
        {
            const auto ParkedStatus = InHandle.Get<FFragment_Sm_DeferredRunStatusMirror>().Get_Status();
            InHandle.Remove<FFragment_Sm_DeferredRunStatusMirror>();
            ck::statemachine::MirrorRunStatus(InHandle, ParkedStatus);
        }
    }

    // ================================================================================================================
    // FLUSH PENDING REPLICATION — DRAIN
    // ================================================================================================================

    auto
        FProcessor_Sm_FlushPendingReplication_Drain::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_PendingReplicationEntries& InStash)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_FlushPendingReplication_Drain);

        auto& StashedEntries = InStash.Get_StashedEntries();

        // Capture pending run-status before we tear the stash fragment down — applying after
        // events drain preserves the on-the-wire ordering (transitions then status).
        const auto HasPendingRunStatus = InStash.Get_HasPendingRunStatus();
        const auto PendingRunStatus    = InStash.Get_PendingRunStatus();

        if (StashedEntries.IsEmpty() && NOT HasPendingRunStatus)
        {
            // Remove the empty fragment so future deliveries don't re-enter stash mode unless
            // explicitly required (the stash-precedence check treats a non-empty stash as
            // "keep stashing"). An empty stash with the fragment still present would force
            // unnecessary stashing on the next OnChange.
            InHandle.Try_Remove<FFragment_Sm_PendingReplicationEntries>();
            return;
        }

        if (NOT StashedEntries.IsEmpty())
        {
            const auto LastApplied = InHandle.Has<FFragment_Sm_ClientReplayState>()
                ? InHandle.Get<FFragment_Sm_ClientReplayState>().Get_ClientLastAppliedSeq()
                : 0;

            auto& Queue = InHandle.AddOrGet<FFragment_Sm_ReplayQueue>().Get_Queue();
            for (const auto& Event : StashedEntries)
            {
                if (Event.Get_Seq() > LastApplied)
                { Queue.Add(Event); }
            }

            StashedEntries.Reset();
        }

        // Stash teardown precedes mirror so MirrorRunStatus's Has<FFragment_Sm_Current> check
        // is the only gate — the now-empty stash fragment is gone.
        InHandle.Try_Remove<FFragment_Sm_PendingReplicationEntries>();

        if (HasPendingRunStatus)
        {
            // The stashed events were only MOVED to the replay queue above — they haven't
            // committed yet. A non-Running status must therefore park until the queue drains
            // (defer-while-replaying), or it would make the commit discard every stashed event.
            ck::statemachine::MirrorRunStatus_OrDeferWhileReplaying(InHandle, PendingRunStatus);
        }
    }

    // ================================================================================================================
    // APPLY REPLICATED HISTORY (non-owning client replay path)
    // ================================================================================================================

    auto
        FProcessor_Sm_ApplyReplicatedHistory::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_ReplayQueue& InQueue)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_ApplyReplicatedHistory);

        auto& Queue = InQueue.Get_Queue();
        if (Queue.IsEmpty())
        { return; }

        // Drain one event per tick. The TExclude<FFragment_Sm_PendingTransition> on this
        // processor means the next entry won't drain until the current transition lands via
        // FProcessor_Sm_CommitPendingTransition (which removes the PendingTransition fragment).
        // That gives the exit/enter cascade time to flush between transitions and matches the
        // server-side per-tick pace.
        const auto Event = Queue[0];
        Queue.RemoveAt(0);

        auto& Pending = InHandle.AddOrGet<FFragment_Sm_PendingTransition>();
        Pending._PreviousStateHandle = InCurrent.Get_CurrentStateHandle();
        Pending._PreviousStateClass  = Event.Get_PreviousStateClass();
        Pending._TargetStateClass    = Event.Get_NewStateClass();
        Pending._NewStateFingerprint = Event.Get_NewStateFingerprint();

        // Watermark the highest seq we've applied. The OnChange handler uses this to filter
        // future delta payloads down to only seqs we haven't yet processed. Max-guarded: a
        // duplicate or out-of-order event must never move the watermark BACKWARD — a regressed
        // watermark re-admits already-applied seqs from the next ring delivery.
        auto& ReplayState = InHandle.AddOrGet<FFragment_Sm_ClientReplayState>();
        ReplayState.Set_ClientLastAppliedSeq(
            FMath::Max(ReplayState.Get_ClientLastAppliedSeq(), Event.Get_Seq()));
    }

    // ================================================================================================================
    // FIRST-SYNC INITIAL STATE
    // ================================================================================================================

    auto
        FProcessor_Sm_FirstSyncInitialState::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_FirstSyncInitialState);

        InHandle.Try_Remove<FTag_Sm_NeedsInitialStateEntry>();

        // WithHistory only. NoHistory SMs snap to the replicated _CurrentStateClass via their own
        // OnChange handler — they never sit at <none> and a first-sync here would race/duplicate it.
        if (InParams.Get_ReplicationModel() != ECk_Sm_ReplicationModel::WithHistory)
        { return; }

        // Idempotence guards: only enter when the SM is actually Running and hasn't already
        // acquired a state via the replay path (RunBefore ApplyReplicatedHistory +
        // TExclude<PendingTransition> normally prevent the race, but stay defensive).
        if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Running)
        { return; }

        if (ck::IsValid(InCurrent.Get_CurrentStateHandle()))
        { return; }

        // Enter the locally-known initial state (DoEnterState resolves it through the override
        // map). No publish: this is a local reconstruction on a non-authority machine, mirroring
        // how the replay path reconstructs transitions.
        FProcessor_Sm_HandleRequests::DoEnterState(InHandle, InCurrent, InParams.Get_InitialStateClass());

        // Initial-entry fire with PreviousStateClass=null, matching the authority's DoStart so
        // non-owning consumers (visuals, per-state logic) get the same clean initial pulse.
        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                TSubclassOf<UCk_SmState_EntityScript>{},
                InCurrent.Get_CurrentStateClass(),
                InCurrent.Get_CurrentStateHandle()
            }));
    }

    // ================================================================================================================
    // HYDRATION RESUME (authority-side, save-load)
    // ================================================================================================================

    auto
        FProcessor_Sm_HydrationResume::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_HydrationResume& InResume) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_HydrationResume);

        // The SM's save-transport Apply (CkStateMachine_Replication.cpp) already stashed InResume with the saved
        // {RunStatus, CurrentStateClass}, and only did so once FFragment_Sm_Current existed — so the fresh SM is
        // normal-boot-composed and there is NO virgin reset (unlike the old Model-A re-drive). Setup may still have
        // AutoStart's Start enqueued behind FTag_Sm_RequiresSetup in the same pump — wait for it to drain first.
        if (InHandle.Has<FTag_Sm_RequiresSetup>())
        { return; }

        auto& Pending = InResume;

        const auto DoMarkResumed = [&]() -> void
        {
            ck::sm::Display(TEXT("[SM HydrationResume] [{}] done — RunStatus [{}] state [{}]"),
                InHandle, InCurrent.Get_RunStatus(),
                ck::IsValid(InCurrent.Get_CurrentStateClass())
                    ? InCurrent.Get_CurrentStateClass()->GetFName()
                    : FName{TEXT("<none>")});

            // Removing the fragment is the done marker — it drops the entity out of this processor's view.
            auto MutableHandle = InHandle;
            MutableHandle.Remove<FFragment_Sm_HydrationResume>();
        };

        // v1 scope ([P4A-F1]): only re-drive where THIS machine passes the same request-authority gate
        // FProcessor_Sm_HandleRequests enforces — otherwise every Request_* below would be ensure-dropped.
        // Non-authority hydrations (OwningClientAuth on a dedicated server / non-owning listen host) come back
        // composed-but-Stopped; authority-side resume is a follow-up design.
        const auto NetContext    = ck::statemachine::ComputeNetContext(InHandle);
        const auto EffectiveAuth = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InHandle);
        const auto IsListenServerHostOwningClient =
            NetContext == ECk_Sm_NetContext::Server
            && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative
            && UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InHandle)
                == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;
        const auto IsRequestAuthority =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || NetContext == ECk_Sm_NetContext::Standalone
            || (NetContext == ECk_Sm_NetContext::Server
                && EffectiveAuth == ECk_Sm_AuthorityModel::ServerAuthoritative)
            || (NetContext == ECk_Sm_NetContext::OwningClient
                && EffectiveAuth == ECk_Sm_AuthorityModel::OwningClientAuthoritative)
            || IsListenServerHostOwningClient;

        if (NOT IsRequestAuthority)
        {
            ck::sm::Log(TEXT("HydrationResume: SM [{}] hydrated on a non-authority machine "
                "(NetContext [{}], AuthorityModel [{}]) — the saved RunStatus [{}] / state are NOT re-driven "
                "(v1 scope cut, [P4A-F1])"),
                InHandle, NetContext, EffectiveAuth, Pending.Get_DesiredRunStatus());
            DoMarkResumed();
            return;
        }

        auto Sm = InHandle;

        switch (Pending.Get_Phase())
        {
            case FFragment_Sm_HydrationResume::EPhase::Start:
            {
                if (Pending.Get_DesiredRunStatus() == ECk_SmRunStatus::Stopped)
                {
                    // Saved Stopped, but AutoStart=OnSetup started the fresh machine — converge back to Stopped
                    // (re-runs InitialState's exit side effects; accepted Option A noise).
                    if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Stopped)
                    {
                        if (NOT Pending.Get_StopEnqueued())
                        {
                            UCk_Utils_StateMachine_UE::Request_Stop(Sm);
                            Pending._StopEnqueued = true;
                        }
                        return;
                    }

                    DoMarkResumed();
                    return;
                }

                if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Running)
                {
                    if (NOT Pending.Get_StartEnqueued())
                    {
                        // Idempotent overlap with AutoStart=OnSetup's own Start — HandleRequests drops
                        // a Start on an already-Running machine.
                        ck::sm::Display(TEXT("[SM HydrationResume] [{}] Start phase: enqueueing Request_Start"), InHandle);
                        UCk_Utils_StateMachine_UE::Request_Start(Sm);
                        Pending._StartEnqueued = true;
                    }
                    return;
                }

                ck::sm::Display(TEXT("[SM HydrationResume] [{}] Start phase: Running observed -> Transition phase"), InHandle);
                Pending._Phase = FFragment_Sm_HydrationResume::EPhase::Transition;
                return;
            }
            case FFragment_Sm_HydrationResume::EPhase::Transition:
            {
                // An in-flight transition (the restore transition, or AutoStart racing a gameplay
                // request) must commit before the ladder advances.
                if (InHandle.Has<FFragment_Sm_PendingTransition>())
                { return; }

                const auto& DesiredClass = Pending.Get_DesiredStateClass();

                // A null desired class means the SM was saved mid-transition (an in-flight transition
                // target is not persisted) or before any state entry; staying in InitialState is the
                // v1 contract for both.
                if (ck::IsValid(DesiredClass) && InCurrent.Get_CurrentStateClass() != DesiredClass)
                {
                    if (NOT Pending.Get_TransitionEnqueued())
                    {
                        ck::sm::Display(TEXT("[SM HydrationResume] [{}] Transition phase: enqueueing Request_Transition -> [{}]"),
                            InHandle, DesiredClass->GetFName());
                        UCk_Utils_StateMachine_UE::Request_Transition(Sm, DesiredClass);
                        Pending._TransitionEnqueued = true;
                    }
                    return;
                }

                ck::sm::Display(TEXT("[SM HydrationResume] [{}] Transition phase: current matches desired -> Finalize phase"), InHandle);
                Pending._Phase = FFragment_Sm_HydrationResume::EPhase::Finalize;
                return;
            }
            case FFragment_Sm_HydrationResume::EPhase::Finalize:
            {
                if (Pending.Get_DesiredRunStatus() == ECk_SmRunStatus::Paused)
                {
                    if (InCurrent.Get_RunStatus() == ECk_SmRunStatus::Running)
                    {
                        if (NOT Pending.Get_PauseEnqueued())
                        {
                            UCk_Utils_StateMachine_UE::Request_Pause(Sm);
                            Pending._PauseEnqueued = true;
                        }
                        return;
                    }

                    if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Paused)
                    { return; }
                }

                DoMarkResumed();
                return;
            }
        }
    }

    // ================================================================================================================
    // PUSH OWNING-CLIENT BATCH (Phase 10 — client→server RPC of locally-buffered transitions)
    // ================================================================================================================

    auto
        FProcessor_Sm_PushOwningClientBatch::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_PendingClientBatch& InBatch)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_PushOwningClientBatch);

        auto& Events = InBatch.Get_PendingEvents();
        const auto HasRunStatus = InBatch.Get_HasPendingRunStatus();

        if (Events.IsEmpty() && NOT HasRunStatus)
        { return; }

        // Spec §5.5 / §5.6: only owning-client-authoritative SMs route through this push, and
        // only on the machine that actually owns the SM's actor. Other machines never accumulate
        // into this batch (the commit + run-status publication paths gate the same way), but the
        // check here is defence in depth — if a misconfigured SM somehow seeded the batch on the
        // wrong machine, we silently clear it rather than emit RPCs that the server would reject.
        if (UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(InHandle) != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        {
            Events.Reset();
            InBatch.Set_HasPendingRunStatus(false);
            return;
        }

        if (UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InHandle)
            != ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled)
        {
            Events.Reset();
            InBatch.Set_HasPendingRunStatus(false);
            return;
        }

        const auto ChannelResult = UCk_Utils_StateMachine_UE::Acquire_RelayChannel(InHandle);
        auto* RelayActor = Cast<ACk_StateMachineRelay_UE>(ChannelResult.Get_ChannelActor().Get());

        // The relay must be RPC-callable from THIS client to push: a client may only invoke a
        // Server_* RPC on an actor it owns (NetConnection resolved → AutonomousProxy). Right after
        // Start the owning channel can still be a SimulatedProxy with no NetConnection (ownership not
        // yet replicated); flushing onto it would have UE silently DROP the RPC, and since we'd then
        // clear the batch the run-status/transition would be lost. Treat "not RPC-ready" exactly like
        // "no relay yet" — defer and retry next pump (the batch is retained).
        if (ck::Is_NOT_Valid(RelayActor, ck::IsValid_Policy_NullptrOnly{})
            || RelayActor->GetNetConnection() == nullptr)
        {
            ck::sm::VeryVerbose(TEXT("PushOwningClientBatch: relay not RPC-ready for [{}], deferring"),
                InHandle);
            return;
        }

        // Run-status FIRST: Start must reach the server before any transition so the server's SM is
        // Running by the time the transition batch lands (ApplyReplicatedHistory drops transitions
        // on a non-Running SM). Wires up Server_PushRunStatus, which was previously never called.
        if (HasRunStatus)
        {
            RelayActor->Server_PushRunStatus(InHandle, InBatch.Get_PendingRunStatus());
            InBatch.Set_HasPendingRunStatus(false);
        }

        if (NOT Events.IsEmpty())
        {
            switch (InParams.Get_ReplicationModel())
            {
                case ECk_Sm_ReplicationModel::WithHistory:
                {
                    // One RPC carries the whole batch — server replays them in order.
                    RelayActor->Server_PushTransitionBatch(InHandle, Events);
                    break;
                }
                case ECk_Sm_ReplicationModel::WithoutHistory:
                {
                    // Only the latest entry matters under WithoutHistory; collapse to a single push.
                    const auto& Latest = Events.Last();
                    RelayActor->Server_PushCurrentState(
                        InHandle,
                        Latest.Get_NewStateClass(),
                        Latest.Get_Seq(),
                        Latest.Get_NewStateFingerprint());
                    break;
                }
            }
            Events.Reset();
        }

        InHandle.Try_Remove<FFragment_Sm_PendingClientBatch>();
    }

    // ================================================================================================================
    // ENDPLAY
    // ================================================================================================================

    auto
        FProcessor_Sm_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Sm_EndPlay);

        ck::sm::VeryVerbose(TEXT("[SM Lifecycle] FProcessor_Sm_EndPlay on SM [{}] — current state [{}]"),
            InHandle, InCurrent._CurrentStateHandle);

        if (ck::IsValid(InCurrent._CurrentStateHandle))
        {
            UCk_Utils_SmState_UE::Request_Exit(InCurrent._CurrentStateHandle);
        }

        // The lifetime cascade destroys the kept-alive previous state with the SM anyway; the
        // shared discard keeps the "abandoning a mid-flight transition" contract uniform.
        UCk_Utils_StateMachine_UE::Request_TryDiscardPendingTransition(InHandle);

        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InCurrent._CurrentStateHandle = FCk_Handle_SmState{};
        InCurrent._CurrentStateClass = nullptr;
    }

    // ================================================================================================================
    // COMMIT PENDING SCRIPT ATTACH
    // ================================================================================================================

    auto
        FProcessor_SmScript_CommitPendingAttach::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmScript_PendingAttach& InPending)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmScript_CommitPendingAttach);

        const auto ScriptClass = InPending.Get_ScriptClass();
        const auto SpawnParams = InPending.Get_SpawnParams();

        InHandle.Remove<FTag_SmScript_PendingAttach>();
        InHandle.Remove<FFragment_SmScript_PendingAttach>();

        if (ck::Is_NOT_Valid(ScriptClass))
        { return; }

        auto MutableHandle = InHandle;
        UCk_Utils_EntityScript_UE::Add(MutableHandle, ScriptClass, SpawnParams);
    }
}

// --------------------------------------------------------------------------------------------------------------------
