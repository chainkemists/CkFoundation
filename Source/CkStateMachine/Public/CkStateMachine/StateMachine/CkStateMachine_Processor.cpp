#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
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
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_ApplyReplicatedHistory);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_CommitPendingTransition);
CK_REGISTER_PROCESSOR(ck::FProcessor_Sm_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmScript_CommitPendingAttach);

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
        // Authority gating (spec §5.6/§6): mutating requests (Start/Stop/Pause/Resume/Transition/
        // AddOverrideState) may only originate on the authority for this SM. Non-authority
        // machines receive state changes via replication and bypass this processor entirely
        // through FProcessor_Sm_ApplyReplicatedHistory (Phase 7). DoesNotReplicate SMs are
        // self-authoritative on every machine — no gating needed.
        const auto NetContext = ck::statemachine::ComputeNetContext(InHandle);
        const auto IsRequestAuthority =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || NetContext == ECk_Sm_NetContext::Standalone
            || (NetContext == ECk_Sm_NetContext::Server
                && InParams.Get_AuthorityModel() == ECk_Sm_AuthorityModel::ServerAuthoritative)
            || (NetContext == ECk_Sm_NetContext::OwningClient
                && InParams.Get_AuthorityModel() == ECk_Sm_AuthorityModel::OwningClientAuthoritative);

        if (NOT IsRequestAuthority)
        {
            CK_ENSURE_IF_NOT(InRequests.Get_Requests().IsEmpty(),
                TEXT("Non-authority machine attempted to enqueue [{}] SM request(s) on [{}] "
                     "(NetContext [{}], AuthorityModel [{}]). Requests dropped. Rep-driven transitions "
                     "bypass this processor via the replay path."),
                InRequests.Get_Requests().Num(), InHandle, NetContext, InParams.Get_AuthorityModel())
            {
                InHandle.Try_Remove<FFragment_Sm_Requests>();
                return;
            }

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

    auto
        FProcessor_Sm_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest)
        -> void
    {
        if (InCurrent._RunStatus == ECk_SmRunStatus::Running)
        { return; }

        InCurrent._RunStatus = ECk_SmRunStatus::Running;
        InHandle.Add<FTag_Sm_Running>();
        InHandle.Try_Remove<FTag_Sm_Paused>();

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
        InHandle.Try_Remove<FFragment_Sm_PendingTransition>();

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
        { return; }

        const auto PreviousStateClass  = InCurrent._CurrentStateClass;
        const auto PreviousStateHandle = InCurrent._CurrentStateHandle;

        UCk_Utils_StateMachine_UE::TryCheckExitBreakpoint(InHandle, PreviousStateClass);

        DoExitCurrentState(InHandle, InCurrent);

        // The actual entry happens in FProcessor_Sm_CommitPendingTransition, after the
        // exit cascade (state -> task -> transition -> condition) has fully drained.
        auto& Pending = InHandle.AddOrGet<FFragment_Sm_PendingTransition>();
        Pending._PreviousStateHandle = PreviousStateHandle;
        Pending._PreviousStateClass  = PreviousStateClass;
        Pending._TargetStateClass    = InRequest.Get_TargetStateClass();

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
            FFragment_Sm_Current& InCurrent)
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

        UCk_Utils_SmState_UE::Request_Exit(InCurrent._CurrentStateHandle);

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
        // RunAfter the full exit cascade ensures Get_IsPendingExit is false here in the
        // normal flow. The check is a safety net for unusual schedules (e.g. an exit
        // request straddling a frame boundary).
        if (UCk_Utils_SmState_UE::Get_IsPendingExit(InPending._PreviousStateHandle))
        { return; }

        if (InCurrent._RunStatus != ECk_SmRunStatus::Running)
        {
            InHandle.Try_Remove<FFragment_Sm_PendingTransition>();
            return;
        }

        const auto PreviousStateClass = InPending._PreviousStateClass;
        const auto TargetStateClass   = InPending._TargetStateClass;

        FProcessor_Sm_HandleRequests::DoEnterState(InHandle, InCurrent, TargetStateClass);

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

        UUtils_Signal_OnSmStateChanged::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Sm_Payload_OnStateChanged{
                PreviousStateClass,
                TargetStateClass,
                InCurrent._CurrentStateHandle
            }));

        UCk_Utils_StateMachine_UE::TryCheckEntryBreakpoint(InHandle, TargetStateClass);

        InHandle.Try_Remove<FFragment_Sm_PendingTransition>();
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

        // Watermark the highest seq we've applied. The OnChange handler uses this to filter
        // future delta payloads down to only seqs we haven't yet processed.
        auto& ReplayState = InHandle.AddOrGet<FFragment_Sm_ClientReplayState>();
        ReplayState.Set_ClientLastAppliedSeq(Event.Get_Seq());
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
        ck::sm::VeryVerbose(TEXT("[SM Lifecycle] FProcessor_Sm_EndPlay on SM [{}] — current state [{}]"),
            InHandle, InCurrent._CurrentStateHandle);

        if (ck::IsValid(InCurrent._CurrentStateHandle))
        {
            UCk_Utils_SmState_UE::Request_Exit(InCurrent._CurrentStateHandle);
        }

        InHandle.Try_Remove<FFragment_Sm_PendingTransition>();

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
