#include "CkTimer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkTimer/CkTimer_Log.h"
#include "CkTimer/CkTimer_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Timer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            FFragment_Timer_Requests& InRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InRequestsComp._Requests;
        InRequestsComp._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequestVariant) -> void
        {
            DoHandleRequest(InDeltaT, InTimerEntity, InCurrentComp, InParamsComp, InRequestVariant);
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
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Manipulate& InRequest)
        -> void
    {
        auto& TimerChrono = InCurrentComp._Chrono;

        switch (const auto& TimeManipulate = InRequest.Get_Manipulate())
        {
            case ECk_Timer_Manipulate::Reset:
            {
                timer::VeryVerbose(TEXT("Handling Reset Request for Timer with Entity [{}]"), InTimerEntity);

                InTimerEntity.AddOrGet<FTag_Timer_NeedsUpdate>();

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                if (InParamsComp.Get_Params().Get_CountDirection() == ECk_Timer_CountDirection::CountUp)
                { TimerChrono.Reset(); }
                else
                { TimerChrono.Complete(); }

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
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
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                if (InParamsComp.Get_Params().Get_CountDirection() == ECk_Timer_CountDirection::CountUp)
                { TimerChrono.Complete(); }
                else
                { TimerChrono.Reset(); }

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Stop:
            {
                timer::VeryVerbose(TEXT("Handling Stop Request for Timer with Entity [{}]"), InTimerEntity);

                InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerStop::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                TimerChrono.Reset();

                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Pause:
            {
                timer::VeryVerbose(TEXT("Handling Pause Request for Timer with Entity [{}]"), InTimerEntity);

                InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerPause::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_Manipulate::Resume:
            {
                timer::VeryVerbose(TEXT("Handling Resume Request for Timer with Entity [{}]"), InTimerEntity);

                InTimerEntity.AddOrGet<FTag_Timer_NeedsUpdate>();
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
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
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Jump& InRequest)
        -> void
    {
        auto& TimerChrono = InCurrentComp._Chrono;

        switch(InParamsComp.Get_Params().Get_CountDirection())
        {
            case ECk_Timer_CountDirection::CountUp:
            {
                TimerChrono.Tick(InRequest.Get_JumpDuration());
                break;
            }
            case ECk_Timer_CountDirection::CountDown:
            {
                TimerChrono.Consume(InRequest.Get_JumpDuration());
                break;
            }
        }

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
        }
    }

    auto
        FProcessor_Timer_HandleRequests::
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Consume& InRequest)
        -> void
    {
        auto& TimerChrono = InCurrentComp._Chrono;

        const auto PreviousTimeElapsed = TimerChrono.Get_TimeElapsed();

        switch(InParamsComp.Get_Params().Get_CountDirection())
        {
            case ECk_Timer_CountDirection::CountUp:
            {
                TimerChrono.Consume(InRequest.Get_ConsumeDuration());

                if (TimerChrono.Get_IsDepleted() && PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerDepleted::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
                }

                break;
            }
            case ECk_Timer_CountDirection::CountDown:
            {
                TimerChrono.Tick(InRequest.Get_ConsumeDuration());

                if (TimerChrono.Get_IsDone() && PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
                {
#if STATS
                    auto TimerStatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif // STATS
                    UUtils_Signal_OnTimerDepleted::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
                }

                break;
            }
        }

        if (PreviousTimeElapsed != TimerChrono.Get_TimeElapsed())
        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InHandle, MakePayload(InHandle, TimerChrono, InDeltaT));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Timer_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp) const
        -> void
    {

        auto& TimerChrono = InCurrentComp._Chrono;

        if (TimerChrono.Get_IsDone() && TimerChrono.Get_GoalValue() > FCk_Time::ZeroSecond())
        { return; }

        timer::VeryVerbose(TEXT("Timer Counting Up with Entity [{}]"), InTimerEntity);

        TimerChrono.Tick(InDeltaT);

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }

        if (NOT TimerChrono.Get_IsDone())
        { return; }

        switch (const auto& TimerBehavior = InParams.Get_Params().Get_Behavior())
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

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity);
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

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity);
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

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity);
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
            auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
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
            FFragment_Timer_Current& InCurrentComp) const
        -> void
    {
        auto& TimerChrono = InCurrentComp._Chrono;

        if (TimerChrono.Get_IsDepleted() && TimerChrono.Get_GoalValue() > FCk_Time::ZeroSecond())
        { return; }

        timer::VeryVerbose(TEXT("Timer Counting Down with Entity [{}]"), InTimerEntity);

        TimerChrono.Consume(InDeltaT);

        {
#if STATS
            auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
            UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }

        if (NOT TimerChrono.Get_IsDepleted())
        { return; }

        switch (const auto& TimerBehavior = InParams.Get_Params().Get_Behavior())
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

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity);
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

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity);
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

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity);
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
            auto TimerStatCounter = FScopeCycleCounter{InTimerEntity.Get<TStatId>()};
#endif // STATS
            UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(InTimerEntity, TimerChrono, InDeltaT));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------