#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
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
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_CancelPendingRequests);
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
    // --------------------------------------------------------------------------------------------------------------------

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

        const auto ShouldAttachReplicatedPayload =
            InParams.Get_Replication() == ECk_Replication::Replicates
            && UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle);

        if (ShouldAttachReplicatedPayload)
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

    // --------------------------------------------------------------------------------------------------------------------

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

        // OwningClientAuthoritative authority = "the machine that locally controls the owning actor".
        // On a LISTEN SERVER the host is both Server (ComputeNetContext short-circuits) and the owning
        // client of its own pawn, so it is the authority too; a dedicated server controls no pawn.
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
            // Every branch below drops the whole batch; a dropped Transition would leave
            // FTag_Sm_TransitionQueued behind and block the evaluator forever.
            InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

            // AutoStart enqueues a Start on EVERY machine, so a dropped Start on a non-authority
            // copy is expected, not misuse — it reaches Running via the replicated run-status. Only
            // the authority-only mutations signal a programming error, so scope the ensure to those.
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
            {}

            // The batch never reaches a handler, so complete every entry or a caller awaiting
            // completion waits forever. Failed rather than Failed_Cancelled: the owner is alive, the
            // request simply was not this machine's to process. Fired off a COPY with the queue
            // already detached, so a delegate is free to enqueue a fresh request from inside.
            InHandle.CopyAndRemove(InRequests, [&](FFragment_Sm_Requests& InRequestsCopy)
            {
                algo::ForEachRequest(InRequestsCopy._Requests, Visitor(
                [&](const auto& InRequest) -> void
                {
                    InRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed);
                }), policy::DontResetContainer{});
            });

            return;
        }

        InHandle.CopyAndRemove(InRequests, [&](FFragment_Sm_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
            {
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                Result = DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Mirrors the SM's run-status into the replicated payload when the local machine is this SM's
    // authority. Local-only and non-authority machines skip.
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

        // The server is the canonical rep publisher regardless of authority model — this is what
        // carries the listen-server host's OwningClientAuth run-status to other clients. A REMOTE
        // owning client buffers for the relay instead so the server can republish.
        const auto IsRepPublisher = NetContext == ECk_Sm_NetContext::Server;

        const auto IsOwningClientOriginator =
            NetContext == ECk_Sm_NetContext::OwningClient
            && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative;

        if (NOT IsRepPublisher && NOT IsOwningClientOriginator)
        { return; }

        // The client doesn't own the server→client rep container (TryUpdateContainerFragment would
        // no-op), so buffer for FProcessor_Sm_PushOwningClientBatch to relay via Server_PushRunStatus.
        if (IsOwningClientOriginator)
        {
            auto& Batch = InSm.AddOrGet<FFragment_Sm_PendingClientBatch>();
            Batch.Set_PendingRunStatus(InNewStatus);
            Batch.Set_HasPendingRunStatus(true);
            return;
        }

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
        -> ECk_Request_OperationResult
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Stopped)
        {
            // Paused is rejected too: falling through would Add a duplicate FTag_Sm_Running and stack
            // a fresh initial state on top of the still-alive paused one. Resume is the way back.
            if (InCurrent._RunStatus == ECk_SmRunStatus::Paused)
            {
                ck::sm::Warning(
                    TEXT("Request_Start on [{}] while Paused — ignored. Use Request_Resume."), InHandle);
            }
            return ECk_Request_OperationResult::Failed;
        }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Add<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Running);

        DoEnterState(InHandle, InCurrent, InParams.Get_InitialStateClass());

        UUtils_Signal_OnSmStarted::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStarted{}));

        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                TSubclassOf<UCk_SmState_EntityScript>{},
                InCurrent._CurrentStateClass,
                InCurrent._CurrentStateHandle
            }));

        UCk_Utils_StateMachine_UE::TryCheckEntryBreakpoint(InHandle, InParams.Get_InitialStateClass());

#if !UE_BUILD_SHIPPING
        // A sub-SM's initial-state entry must be surfaced in the PARENT's history: sub-SM history is
        // only synthesized from transitions, so one that enters and dies in the same frame would
        // otherwise leave no trace in the debugger.
        if (TUtils_Sm_OwningStateMachine::Has(InHandle))
        {
            auto ParentSm = TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);

            if (UCk_Utils_StateMachineDebug_UE::Get_IsDebuggerCaptureActive(ParentSm))
            {
                auto ParentStateName = FString{};
                if (ck::IsValid(ParentSm) && ParentSm.Has<FFragment_Sm_Current>())
                {
                    if (const auto ParentCurrentClass = ParentSm.Get<FFragment_Sm_Current>().Get_CurrentStateClass();
                        ck::IsValid(ParentCurrentClass))
                    {
                        ParentStateName = UCk_Utils_Object_UE::Get_CleanClassName(ParentCurrentClass);
                    }
                }

                // _CurrentStateClass is already override-resolved by DoEnterState; the params'
                // initial state class is the pre-resolution request and would display the base class.
                auto SubSmStartRequest = FCk_Request_SmDebug_RecordTransition{
                    TSubclassOf<UCk_SmState_EntityScript>{}, InCurrent._CurrentStateClass};
                SubSmStartRequest.Set_FrameNumber(UCk_Utils_Time_UE::Get_FrameNumber());
                SubSmStartRequest.Set_RealTimeSeconds(FPlatformTime::Seconds());
                SubSmStartRequest.Set_SubSmParentStateName(ParentStateName);

                UCk_Utils_StateMachineDebug_UE::Request_RecordTransition(ParentSm, SubSmStartRequest);
            }
        }
#endif

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent._RunStatus == ECk_SmRunStatus::Stopped)
        { return ECk_Request_OperationResult::Failed; }

        DoExitCurrentState(InHandle, InCurrent);

        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InHandle.Try_Remove<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();
        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

        // A transition can be mid-flight (exit cascade ran, commit hasn't). HandleRequests runs before
        // CommitPendingTransition, so discarding here is the only chance — otherwise the kept-alive
        // previous state leaks.
        UCk_Utils_StateMachine_UE::Request_TryDiscardPendingTransition(InHandle);

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Stopped);

        UUtils_Signal_OnSmStopped::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStopped{}));

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        { return ECk_Request_OperationResult::Failed; }

        InCurrent._RunStatus = ECk_SmRunStatus::Paused;
        InHandle.Add<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Paused);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Paused)
        { return ECk_Request_OperationResult::Failed; }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Remove<FTag_Sm_Paused>();

        DoPublishRunStatus(InHandle, InParams, ECk_SmRunStatus::Running);

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest)
        -> ECk_Request_OperationResult
    {
        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        {
            // The enqueue side unconditionally added FTag_Sm_TransitionQueued; a dropped request must
            // clear it or FProcessor_SmState_Evaluate blocks forever.
            InHandle.Try_Remove<FTag_Sm_TransitionQueued>();
            return ECk_Request_OperationResult::Failed;
        }

        const auto PreviousStateClass  = InCurrent._CurrentStateClass;
        const auto PreviousStateHandle = InCurrent._CurrentStateHandle;

        UCk_Utils_StateMachine_UE::TryCheckExitBreakpoint(InHandle, PreviousStateClass);

        // The previous state must stay alive until FProcessor_Sm_CommitPendingTransition reads its
        // handle: destroying here (deferred to end-of-frame) can win the race against a commit whose
        // exit cascade straddles a frame boundary, leaving the commit querying a tombstone.
        constexpr auto ScheduleDestroyNow = false;
        DoExitCurrentState(InHandle, InCurrent, ScheduleDestroyNow);

        // The entry itself happens in FProcessor_Sm_CommitPendingTransition. A second
        // Request_Transition can land before the first commits: keep the ORIGINAL previous-state
        // stash (the only reference to the kept-alive exited state) and only retarget, so the
        // never-entered intermediate target is skipped and observers see one A→C transition.
        const auto HadPendingTransition = InHandle.Has<FFragment_Sm_PendingTransition>();
        auto& Pending = InHandle.AddOrGet<FFragment_Sm_PendingTransition>();
        if (NOT HadPendingTransition)
        {
            Pending._PreviousStateHandle = PreviousStateHandle;
            Pending._PreviousStateClass  = PreviousStateClass;
        }
        Pending._TargetStateClass = InRequest.Get_TargetStateClass();

        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_AddOverrideState& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& OverrideClass = InRequest.Get_OverrideStateClass();

        const auto OverrideClassIsValid = ck::IsValid(OverrideClass);

        CK_ENSURE_IF_NOT(OverrideClassIsValid,
            TEXT("FCk_Request_Sm_AddOverrideState on [{}] has invalid override state class"), InHandle)
        {}
        if (NOT OverrideClassIsValid)
        { return ECk_Request_OperationResult::Failed; }

        auto* CDO = OverrideClass->GetDefaultObject<UCk_SmState_EntityScript>();
        const auto StatesToOverride = CDO->Get_StatesToOverride();

        const auto HasStatesToOverride = StatesToOverride.Num() > 0;

        CK_ENSURE_IF_NOT(HasStatesToOverride,
            TEXT("FCk_Request_Sm_AddOverrideState on [{}]: override class [{}] returns empty Get_StatesToOverride()"),
            InHandle, *OverrideClass->GetName())
        {}
        if (NOT HasStatesToOverride)
        { return ECk_Request_OperationResult::Failed; }

        auto& Overrides = InHandle.AddOrGet<FFragment_Sm_StateOverrides>();
        Overrides._Overrides.Add(FFragment_Sm_StateOverrides::FEntry{OverrideClass, StatesToOverride});

        return ECk_Request_OperationResult::Succeeded;
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

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Sm_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // The previous state was kept ALIVE by DoExitCurrentState(ScheduleDestroyNow=false) and is
        // destroyed below, after the new state is entered. Wait while it is still mid-exit; the
        // validity guard covers the initial transition and any caller that released it.
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

        // Safe to destroy the state the transition path deliberately kept alive: its exit lifecycle
        // already ran via FProcessor_SmState_Exit, and EndPlay's ExitState is a no-op (Active dedup).
        if (ck::IsValid(PreviousStateHandle))
        {
            auto PreviousToDestroy = FCk_Handle{PreviousStateHandle};
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(PreviousToDestroy);
        }

        // Pass the replicated fingerprint expectation to the new state so its Construct /
        // DoComputeFingerprint cycle can verify against it. Authority-driven commits leave it at 0 —
        // no carrier, no verify. The fragment lives just long enough for the deferred verify pass.
        if (IncomingNewFingerprint != 0 && ck::IsValid(InCurrent._CurrentStateHandle))
        {
            auto& Expected = InCurrent._CurrentStateHandle.AddOrGet<FFragment_SmState_ExpectedFingerprint>();
            Expected.Set_Hash(IncomingNewFingerprint);
        }

#if !UE_BUILD_SHIPPING
        if (ck::IsValid(PreviousStateClass) && ck::IsValid(InCurrent._CurrentStateClass))
        {
            if (NOT UCk_Utils_StateMachineDebug_UE::Get_IsDebuggerCaptureActive(InHandle))
            {
                InHandle.Try_Remove<FFragment_Sm_Debug_LastFiredTransition>();
            }
            else
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

        // Two disjoint publish paths; a non-owning client hits neither and only consumes rep deltas.
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

                // Populated by UCk_SmState_EntityScript::DoComputeFingerprint after a state class's
                // first Construct anywhere in this process. The very first instantiation reads 0;
                // DoBackfillFingerprintToRepData cleans up that single zero asynchronously.
                auto NewStateFingerprint =
                    UCk_SmState_EntityScript::Get_CachedFingerprint(TargetStateClass);

#if WITH_DEV_AUTOMATION_TESTS
                // Armed via UCk_Utils_StateMachine_Test_UE::Test_InjectFakeFingerprint: replaces the
                // cached value before it reaches the wire, so the receive side runs the genuine verify.
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

        // Sub-SM transition relay: a sub-SM is DoesNotReplicate, so it has no transport of its own and
        // the publish block above is skipped for it. Its transitions ride the ROOT's client->server
        // batch / rep container, tagged with the sub-SM's parent-hierarchy identity so peers dispatch
        // to the matching local sub-SM.
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
                // Leg 2 (server -> non-owning clients): the server is never the transition authority for
                // an OwningClientAuth sub-SM, so every sub-SM transition it commits is a relayed one.
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
                            // Leg 1 (owning client -> server): buffer onto the root's batch for the push
                            // processor to RPC via Server_PushTransitionBatch.
                            auto& Batch = Root.AddOrGet<FFragment_Sm_PendingClientBatch>();
                            Batch.Get_PendingEvents().Add(Event);
                        }
                        else // IsServerRepublisher
                        {
                            // Republish onto the ROOT's WithHistory container so non-owning observers
                            // dispatch by identity; the owning client echo-suppresses it. NoHistory roots
                            // have no field to carry the identity, so the relay is WithHistory-only.
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

        // Apply a parked run-status mirror once the replay pipeline drains — preserves the authority's
        // transitions-then-status ordering (see FFragment_Sm_DeferredRunStatusMirror).
        if (InHandle.Has<FFragment_Sm_DeferredRunStatusMirror>()
            && NOT UCk_Utils_StateMachine_UE::Get_HasReplayTransitionsInFlight(InHandle))
        {
            const auto ParkedStatus = InHandle.Get<FFragment_Sm_DeferredRunStatusMirror>().Get_Status();
            InHandle.Remove<FFragment_Sm_DeferredRunStatusMirror>();
            ck::statemachine::MirrorRunStatus(InHandle, ParkedStatus);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // Captured before the stash fragment is torn down; applied after events drain so the
        // on-the-wire ordering (transitions then status) is preserved.
        const auto HasPendingRunStatus = InStash.Get_HasPendingRunStatus();
        const auto PendingRunStatus    = InStash.Get_PendingRunStatus();

        if (StashedEntries.IsEmpty() && NOT HasPendingRunStatus)
        {
            // The stash-precedence check treats a present fragment as "keep stashing", so an empty
            // one left behind would force unnecessary stashing on the next OnChange.
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

        // Teardown precedes the mirror so MirrorRunStatus's Has<FFragment_Sm_Current> check is the
        // only gate.
        InHandle.Try_Remove<FFragment_Sm_PendingReplicationEntries>();

        if (HasPendingRunStatus)
        {
            // The stashed events were only MOVED to the replay queue, not committed, so a non-Running
            // status must park until the queue drains or the commit would discard every one of them.
            ck::statemachine::MirrorRunStatus_OrDeferWhileReplaying(InHandle, PendingRunStatus);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // One event per tick: TExclude<FFragment_Sm_PendingTransition> keeps the next entry from
        // draining until the current transition lands, giving the exit/enter cascade time to flush
        // and matching the server-side per-tick pace.
        const auto Event = Queue[0];
        Queue.RemoveAt(0);

        auto& Pending = InHandle.AddOrGet<FFragment_Sm_PendingTransition>();
        Pending._PreviousStateHandle = InCurrent.Get_CurrentStateHandle();
        Pending._PreviousStateClass  = Event.Get_PreviousStateClass();
        Pending._TargetStateClass    = Event.Get_NewStateClass();
        Pending._NewStateFingerprint = Event.Get_NewStateFingerprint();

        // Max-guarded: a duplicate or out-of-order event must never move the watermark BACKWARD, or
        // the next ring delivery re-admits already-applied seqs.
        auto& ReplayState = InHandle.AddOrGet<FFragment_Sm_ClientReplayState>();
        ReplayState.Set_ClientLastAppliedSeq(
            FMath::Max(ReplayState.Get_ClientLastAppliedSeq(), Event.Get_Seq()));
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // Idempotence guards: only enter when the SM is actually Running and hasn't already acquired
        // a state via the replay path.
        if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Running)
        { return; }

        if (ck::IsValid(InCurrent.Get_CurrentStateHandle()))
        { return; }

        // No publish: a local reconstruction on a non-authority machine, mirroring the replay path.
        FProcessor_Sm_HandleRequests::DoEnterState(InHandle, InCurrent, InParams.Get_InitialStateClass());

        // Initial-entry fire with PreviousStateClass=null, matching the authority's DoStart.
        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                TSubclassOf<UCk_SmState_EntityScript>{},
                InCurrent.Get_CurrentStateClass(),
                InCurrent.Get_CurrentStateHandle()
            }));
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // Setup may still have AutoStart's Start enqueued behind FTag_Sm_RequiresSetup in the same
        // pump — wait for it to drain first.
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

        // Only re-drive where THIS machine passes the same request-authority gate
        // FProcessor_Sm_HandleRequests enforces; otherwise every Request_* below is ensure-dropped.
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
                "(NetContext [{}], AuthorityModel [{}]) — the saved RunStatus [{}] / state are NOT re-driven"),
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
                    // AutoStart=OnSetup may have started the fresh machine — converge back to Stopped.
                    if (InCurrent.Get_RunStatus() != ECk_SmRunStatus::Stopped)
                    {
                        if (NOT Pending.Get_StopEnqueued())
                        {
                            UCk_Utils_StateMachine_UE::Request_Stop(Sm, {});
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
                        // Idempotent against AutoStart's own Start — HandleRequests drops a Start on an
                        // already-Running machine.
                        ck::sm::Display(TEXT("[SM HydrationResume] [{}] Start phase: enqueueing Request_Start"), InHandle);
                        UCk_Utils_StateMachine_UE::Request_Start(Sm, {});
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
                // An in-flight transition must commit before the ladder advances.
                if (InHandle.Has<FFragment_Sm_PendingTransition>())
                { return; }

                const auto& DesiredClass = Pending.Get_DesiredStateClass();

                // A null desired class means the SM was saved mid-transition or before any state
                // entry; staying in InitialState is the contract for both.
                if (ck::IsValid(DesiredClass) && InCurrent.Get_CurrentStateClass() != DesiredClass)
                {
                    if (NOT Pending.Get_TransitionEnqueued())
                    {
                        ck::sm::Display(TEXT("[SM HydrationResume] [{}] Transition phase: enqueueing Request_Transition -> [{}]"),
                            InHandle, DesiredClass->GetFName());
                        UCk_Utils_StateMachine_UE::Request_Transition(Sm, DesiredClass, {});
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
                            UCk_Utils_StateMachine_UE::Request_Pause(Sm, {});
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

    // --------------------------------------------------------------------------------------------------------------------

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

        // Defence in depth: the commit + run-status publication paths already gate the same way, so a
        // batch on the wrong machine means a misconfigured SM — clear it rather than emit RPCs the
        // server would reject.
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

        // A client may only invoke a Server_* RPC on an actor it owns (NetConnection resolved →
        // AutonomousProxy). Right after Start the channel can still be a SimulatedProxy and UE would
        // silently DROP the RPC, so treat "not RPC-ready" like "no relay yet" and retry with the
        // batch retained.
        if (ck::Is_NOT_Valid(RelayActor, ck::IsValid_Policy_NullptrOnly{})
            || RelayActor->GetNetConnection() == nullptr)
        {
            ck::sm::VeryVerbose(TEXT("PushOwningClientBatch: relay not RPC-ready for [{}], deferring"),
                InHandle);
            return;
        }

        // Run-status FIRST: Start must reach the server before any transition, since
        // ApplyReplicatedHistory drops transitions on a non-Running SM.
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

    // --------------------------------------------------------------------------------------------------------------------

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

        // Redundant with the lifetime cascade, but keeps the "abandon a mid-flight transition"
        // contract uniform across every abandoning path.
        UCk_Utils_StateMachine_UE::Request_TryDiscardPendingTransition(InHandle);

        InCurrent._RunStatus = ECk_SmRunStatus::Stopped;
        InCurrent._CurrentStateHandle = FCk_Handle_SmState{};
        InCurrent._CurrentStateClass = nullptr;
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        InHandle.Remove<FTag_SmScript_PendingAttach>();
        InHandle.Remove<FFragment_SmScript_PendingAttach>();

        if (ck::Is_NOT_Valid(ScriptClass))
        { return; }

        auto MutableHandle = InHandle;
        UCk_Utils_EntityScript_UE::Add(MutableHandle, ScriptClass, FInstancedStruct{});
    }
}

// --------------------------------------------------------------------------------------------------------------------
