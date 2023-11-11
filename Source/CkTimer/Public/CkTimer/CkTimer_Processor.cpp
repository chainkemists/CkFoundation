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
        auto& timerChrono = InCurrentComp._Chrono;
        const auto& timerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InTimerEntity);

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

                    UUtils_Signal_OnTimerReset::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    timerChrono.Reset();

                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Stop:
                {
                    timer::VeryVerbose(TEXT("Handling Stop Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
                    InTimerEntity.Add<FTag_Timer_Updated>();

                    UUtils_Signal_OnTimerStop::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    timerChrono.Reset();

                    UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Pause:
                {
                    timer::VeryVerbose(TEXT("Handling Pause Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Remove<FTag_Timer_NeedsUpdate>();
                    UUtils_Signal_OnTimerPause::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    break;
                }
                case ECk_Timer_Manipulate::Resume:
                {
                    timer::VeryVerbose(TEXT("Handling Resume Request for Timer with Entity [{}]"), InTimerEntity);

                    InTimerEntity.Add<FTag_Timer_NeedsUpdate>();
                    UUtils_Signal_OnTimerResume::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(timeManipulate);
                    break;
                }
            }
        });

        InTimerEntity.Remove<MarkedDirtyBy>();
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
        auto& timerChrono = InCurrentComp._Chrono;

        if (timerChrono.Get_IsDone() && timerChrono.Get_GoalValue() > FCk_Time::ZeroSecond())
        { return; }

        timer::VeryVerbose(TEXT("Ticking Timer with Entity [{}]"), InTimerEntity);

        timerChrono.Tick(InDeltaT);

        const auto& timerOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InTimerEntity);

        UUtils_Signal_OnTimerUpdate::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));

        InTimerEntity.Add<FTag_Timer_Updated>();

        if (NOT timerChrono.Get_IsDone())
        { return; }

        switch (const auto& timerBehavior = InParams.Get_Params().Get_Behavior())
        {
            case ECk_Timer_Behavior::StopOnDone:
            {
                timer::VeryVerbose
                (
                    TEXT("Timer for Entity [{}] with Chrono [{}] reached Goal Value. Requesting Stop as per its Behavior [{}]"),
                    InTimerEntity,
                    timerChrono,
                    timerBehavior
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
                    timerChrono,
                    timerBehavior
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
                    timerChrono,
                    timerBehavior
                );

                UCk_Utils_Timer_UE::Request_Pause(InTimerEntity, UCk_Utils_GameplayLabel_UE::Get_Label(InTimerEntity));
                break;
            }
            default:
            {
                CK_INVALID_ENUM(timerBehavior);
                break;
            }
        }

        UUtils_Signal_OnTimerDone::Broadcast(InTimerEntity, MakePayload(timerOwningEntity, timerChrono));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Timer_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Timer_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Timer_Rep>& InComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Timer_Rep>(InHandle, [&](UCk_Fragment_Timer_Rep* InRepComp)
        {
            InRepComp->_Chrono = InCurrent.Get_Chrono();
        });

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
