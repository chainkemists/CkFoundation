#include "CkTimer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkTimer/CkTimer_Log.h"
#include "CkTimer/CkTimer_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Update_Countdown);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Timer_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer& InTimerComp)
        -> void
    {
        InTimerEntity.Remove<MarkedDirtyBy>();

        if (InTimerEntity.Has<FTag_Timer_Countdown>())
        {
            InTimerComp._Chrono.Complete();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Timer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer& InTimerComp,
            FFragment_Timer_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            // Every DoHandleRequest overload below is void and has no rejection path, so reaching the
            // line after the call IS the success condition.
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InTimerEntity, Result);

            DoHandleRequest(InDeltaT, InTimerEntity, InTimerComp, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequestsComp._Requests.IsEmpty())
        {
            InTimerEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Timer_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer& InTimerComp,
            const FCk_Request_Timer_Manipulate& InRequest)
        -> void
    {
        auto& TimerChrono = InTimerComp._Chrono;

        switch (const auto& TimeManipulate = InRequest.Get_Manipulate())
        {
            case ECk_Timer_Manipulate::Reset:
            {
                timer::VeryVerbose(TEXT("Handling Reset Request for Timer with Entity [{}]"), InTimerEntity);

                InTimerEntity.AddOrGet<FTag_Timer_NeedsUpdate>();

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                if (InTimerEntity.Has<FTag_Timer_Countdown>())
                { TimerChrono.Complete(); }
                else
                { TimerChrono.Reset(); }

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Complete:
            {
                timer::VeryVerbose(TEXT("Handling Complete Request for Timer with Entity [{}]"), InTimerEntity);

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                if (InTimerEntity.Has<FTag_Timer_Countdown>())
                { TimerChrono.Reset(); }
                else
                { TimerChrono.Complete(); }

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Stop:
            {
                timer::VeryVerbose(TEXT("Handling Stop Request for Timer with Entity [{}]"), InTimerEntity);

                if (InTimerEntity.Try_Remove<FTag_Timer_NeedsUpdate>())
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerStop::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                TimerChrono.Reset();
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Pause:
            {
                timer::VeryVerbose(TEXT("Handling Pause Request for Timer with Entity [{}]"), InTimerEntity);

                if (InTimerEntity.Try_Remove<FTag_Timer_NeedsUpdate>())
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerPause::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Resume:
            {
                timer::VeryVerbose(TEXT("Handling Resume Request for Timer with Entity [{}]"), InTimerEntity);

                if (NOT InTimerEntity.Has<FTag_Timer_NeedsUpdate>())
                {
                    InTimerEntity.Add<FTag_Timer_NeedsUpdate>();
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
                    UUtils_Signal_OnTimerResume::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            default:
            {
                CK_INVALID_ENUM(TimeManipulate);
                break;
            }
        }
    }

    auto
        FProcessor_Timer_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer& InTimerComp,
            const FCk_Request_Timer_Jump& InRequest)
        -> void
    {
        auto& TimerChrono = InTimerComp._Chrono;

        // The single source of truth for jump math — the save/load HydrationApply drives an absolute jump
        // through here.
        const auto RequestedSeconds      = InRequest.Get_JumpDuration().Get_Seconds();
        const auto IsAbsolute            = InRequest.Get_JumpMode() == ECk_RelativeAbsolute::Absolute;
        const auto CurrentElapsedSeconds = TimerChrono.Get_TimeElapsed().Get_Seconds();

        auto DeltaToApply = RequestedSeconds;

        if (InHandle.Has<FTag_Timer_Countdown>())
        {
            // Consume moves elapsed the opposite way; the absolute delta is Current - Target.
            DeltaToApply = IsAbsolute ? CurrentElapsedSeconds - RequestedSeconds : RequestedSeconds;
            TimerChrono.Consume(FCk_Time{DeltaToApply});
        }
        else
        {
            // DeltaToApply is the absolute net movement (Target - Current), used for the signal below.
            DeltaToApply = IsAbsolute ? RequestedSeconds - CurrentElapsedSeconds : RequestedSeconds;
            if (IsAbsolute)
            {
                // Tick early-outs on an already-Done chrono, so a BACKWARD absolute jump from GoalValue would
                // silently no-op — Reset first, then Tick to the target. 0 -> Target is atomic here.
                TimerChrono.Reset();
                TimerChrono.Tick(FCk_Time{RequestedSeconds});
            }
            else
            {
                TimerChrono.Tick(FCk_Time{DeltaToApply});
            }
        }

        {
            const auto& JumpDirection = DeltaToApply >= 0.f
                                        ? ECk_Timer_JumpDirection::Forwards
                                        : ECk_Timer_JumpDirection::Backwards;
            const auto& JumpAmount = FCk_Time(FMath::Abs(DeltaToApply));
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InHandle)};
#endif // STATS
            UUtils_Signal_OnTimerJump::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT, JumpDirection, JumpAmount));
        }
        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InHandle)};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
        }
    }

    auto
        FProcessor_Timer_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer& InTimerComp,
            const FCk_Request_Timer_Consume& InRequest)
        -> void
    {
        auto& TimerChrono = InTimerComp._Chrono;

        const auto PreviousTimeElapsed = TimerChrono.Get_TimeElapsed();

        if (InHandle.Has<FTag_Timer_Countdown>())
        {
            TimerChrono.Tick(InRequest.Get_ConsumeDuration());

            if (TimerChrono.Get_IsDone() && PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
            {
#if STATS
                auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InHandle)};
#endif // STATS
                UUtils_Signal_OnTimerDepleted::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
            }
        }
        else
        {
            TimerChrono.Consume(InRequest.Get_ConsumeDuration());

            if (TimerChrono.Get_IsDepleted() && PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
            {
#if STATS
                auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InHandle)};
#endif // STATS
                UUtils_Signal_OnTimerDepleted::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
            }
        }

        if (PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InHandle)};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Timer_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InTimerEntity, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Timer_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer& InTimerComp) const
        -> void
    {

        auto& TimerChrono = InTimerComp._Chrono;

        if (TimerChrono.Get_IsDone() && TimerChrono.Get_GoalValue() > FCk_Time::ZeroSecond())
        { return; }

        timer::VeryVerbose(TEXT("Timer Counting Up with Entity [{}]"), InTimerEntity);

        TimerChrono.Tick(InDeltaT);

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }

        if (NOT TimerChrono.Get_IsDone())
        { return; }

        switch (const auto& TimerBehavior = InParams.Get_Behavior())
        {
            case ECk_Timer_Behavior::StopOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Stop as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity, {});
                break;
            }
            case ECk_Timer_Behavior::ResetOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Reset as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity, {});
                break;
            }
            case ECk_Timer_Behavior::PauseOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Pause as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity, {});
                break;
            }
            default:
            {
                CK_INVALID_ENUM(TimerBehavior);
                break;
            }
        }

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
            UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }
    }

    auto
        FProcessor_Timer_Update_Countdown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer& InTimerComp) const
        -> void
    {
        auto& TimerChrono = InTimerComp._Chrono;

        if (TimerChrono.Get_IsDepleted() && TimerChrono.Get_GoalValue() > FCk_Time::ZeroSecond())
        { return; }

        timer::VeryVerbose(TEXT("Timer Counting Down with Entity [{}]"), InTimerEntity);

        TimerChrono.Consume(InDeltaT);

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }

        if (NOT TimerChrono.Get_IsDepleted())
        { return; }

        switch (const auto& TimerBehavior = InParams.Get_Behavior())
        {
            case ECk_Timer_Behavior::StopOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Stop as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity, {});
                break;
            }
            case ECk_Timer_Behavior::ResetOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Reset as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity, {});
                break;
            }
            case ECk_Timer_Behavior::PauseOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Pause as per its Behavior [{}]"),
                    InTimerEntity,
                    TimerChrono,
                    TimerBehavior
                );

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity, {});
                break;
            }
            default:
            {
                CK_INVALID_ENUM(TimerBehavior);
                break;
            }
        }

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{MakeStatId(InTimerEntity)};
#endif // STATS
            UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
