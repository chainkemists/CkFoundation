#include "CkTimer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
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
<<<<<<< Updated upstream
        auto& TimerChrono = InCurrentComp._Chrono;
        const auto& TimerLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTimerEntity);

        ck::algo::ForEachRequest(InRequestsComp._ManipulateRequests,
        [&](const FFragment_Timer_Requests::RequestType& InRequest) -> void
        {
            switch (const auto& timeManipulate = InRequest.Get_Manipulate())
            {
                case ECk_Timer_Manipulate::Reset:
                {
                    timer::VeryVerbose(TEXT("Handling Reset Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Add<FTag_Timer_NeedsUpdate>();
                    InTimerEntity.Add<FTag_Timer_Updated>();

                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    TimerChrono.Reset();

                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Stop:
                {
                    timer::VeryVerbose(TEXT("Handling Stop Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
                    InTimerEntity.Add<FTag_Timer_Updated>();

                    UUtils_Signal_OnTimerStop::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    TimerChrono.Reset();

                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Pause:
                {
                    timer::VeryVerbose(TEXT("Handling Pause Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
                    UUtils_Signal_OnTimerPause::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Resume:
                {
                    timer::VeryVerbose(TEXT("Handling Resume Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Add<FTag_Timer_NeedsUpdate>();
                    UUtils_Signal_OnTimerResume::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, TimerChrono));

                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(timeManipulate);
                    break;
                }
            }
        });
=======
        ck::algo::ForEachRequest(InRequestsComp._ManipulateRequests, ck::Visitor(
        [&](const auto& InRequestVariant) -> void
        {
            DoHandleRequest(InTimerEntity, InCurrentComp, InParamsComp, InRequestVariant);
        }));
>>>>>>> Stashed changes

        InTimerEntity.Remove<MarkedDirtyBy>();
    }

    auto
	    FProcessor_Timer_HandleRequests::
		DoHandleRequest(
		    HandleType InTimerEntity,
		    FFragment_Timer_Current& InCurrentComp,
		    const FFragment_Timer_Params& InParamsComp,
		    const FCk_Request_Timer_Manipulate& InRequest) const
		-> void
    {
        auto& TimerChrono = InCurrentComp._Chrono;
        const auto& TimerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InTimerEntity);

		switch (const auto& TimeManipulate = InRequest.Get_Manipulate())
		{
			case ECk_Timer_Manipulate::Reset:
			{
				timer::VeryVerbose(TEXT("Handling Reset Request for Timer with Entity [{}]"), InTimerEntity);

				InTimerEntity.AddOrGet<FTag_Timer_NeedsUpdate>();
				InTimerEntity.AddOrGet<FTag_Timer_Updated>();

				UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

                if (InParamsComp.Get_Params().Get_CountDirection() == ECk_Timer_CountDirection::CountUp)
				{ TimerChrono.Reset(); }
				else
				{ TimerChrono.Complete(); }

				UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

				break;
			}
			case ECk_Timer_Manipulate::Stop:
			{
				timer::VeryVerbose(TEXT("Handling Stop Request for Timer with Entity [{}]"), InTimerEntity);

				InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
				InTimerEntity.AddOrGet<FTag_Timer_Updated>();

				UUtils_Signal_OnTimerStop::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

				TimerChrono.Reset();

				UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

				break;
			}
			case ECk_Timer_Manipulate::Pause:
			{
				timer::VeryVerbose(TEXT("Handling Pause Request for Timer with Entity [{}]"), InTimerEntity);

				InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
				UUtils_Signal_OnTimerPause::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

				break;
			}
			case ECk_Timer_Manipulate::Resume:
			{
				timer::VeryVerbose(TEXT("Handling Resume Request for Timer with Entity [{}]"), InTimerEntity);

				InTimerEntity.AddOrGet<FTag_Timer_NeedsUpdate>();
				UUtils_Signal_OnTimerResume::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

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
		    HandleType InHandle,
		    FFragment_Timer_Current& InCurrentComp,
		    const FFragment_Timer_Params& InParamsComp,
		    const FCk_Request_Timer_Jump& InRequest) const
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

<<<<<<< Updated upstream
        const auto& TimerLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InTimerEntity);

        UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, timerChrono));
=======
        const auto& TimerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InTimerEntity);

        UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));
>>>>>>> Stashed changes

        InTimerEntity.AddOrGet<FTag_Timer_Updated>();

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

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
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

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
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

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
                break;
            }
            default:
            {
                CK_INVALID_ENUM(TimerBehavior);
                break;
            }
        }

<<<<<<< Updated upstream
        UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(TimerLifetimeOwner, timerChrono));
=======
        UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));
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

        const auto& TimerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InTimerEntity);

        UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));

        InTimerEntity.AddOrGet<FTag_Timer_Updated>();

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

                UCk_Utils_Timer_UE::Request_Stop(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
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

                UCk_Utils_Timer_UE::Request_Reset(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
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

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
                break;
            }
            default:
            {
                CK_INVALID_ENUM(TimerBehavior);
                break;
            }
        }

        UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(TimerOwningEntity, TimerChrono));
>>>>>>> Stashed changes
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
