#include "CkStateMachine_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkStateMachine/CkStateMachine_Log.h"
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

        const auto PreviousStateClass = InCurrent._CurrentStateClass;

        UCk_Utils_StateMachine_UE::TryCheckExitBreakpoint(InHandle, PreviousStateClass);

        DoExitCurrentState(InHandle, InCurrent);
        DoEnterState(InHandle, InCurrent, InRequest.Get_TargetStateClass());

        InHandle.Try_Remove<FTag_Sm_TransitionQueued>();

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
                InRequest.Get_TargetStateClass(),
                InCurrent._CurrentStateHandle
            }));

        UCk_Utils_StateMachine_UE::TryCheckEntryBreakpoint(InHandle, InRequest.Get_TargetStateClass());
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
