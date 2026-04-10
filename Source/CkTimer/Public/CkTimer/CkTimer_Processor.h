#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkTimer/CkTimer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTIMER_API FProcessor_Timer_Setup : public ck_exp::TProcessor<
        FProcessor_Timer_Setup,
        FCk_Handle_Timer,
        ck::TReadOnly<FFragment_Timer_Params>,
        ck::TReadWrite<FFragment_Timer_Current>,
        FTag_Timer_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_Timer_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Timer_HandleRequests, FCk_Handle_Timer,
            ck::TReadWrite<FFragment_Timer_Current>, ck::TReadOnly<FFragment_Timer_Params>, ck::TReadWrite<FFragment_Timer_Requests>,
            TExclude<FTag_Timer_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Timer_Setup>;
        using MarkedDirtyBy = FFragment_Timer_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            FFragment_Timer_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Manipulate& InRequest) -> void;

        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Jump& InRequest) -> void;

        static auto
        DoHandleRequest(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Consume& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_Update : public ck_exp::TProcessor<
        FProcessor_Timer_Update,
        FCk_Handle_Timer,
        ck::TReadOnly<FFragment_Timer_Params>,
        ck::TReadWrite<FFragment_Timer_Current>,
        FTag_Timer_NeedsUpdate,
        TExclude<FTag_Timer_NeedsSetup>,
        TExclude<FTag_Timer_Countdown>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Timer_HandleRequests>;
        using MarkedDirtyBy = FTag_Timer_NeedsUpdate;
        using HandleType = FCk_Handle_Timer;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_Update_Countdown
        : public ck_exp::TProcessor<FProcessor_Timer_Update_Countdown, FCk_Handle_Timer, ck::TReadOnly<FFragment_Timer_Params>,
            ck::TReadWrite<FFragment_Timer_Current>, FTag_Timer_NeedsUpdate, FTag_Timer_Countdown,
            TExclude<FTag_Timer_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Timer_HandleRequests>;
        using MarkedDirtyBy = FTag_Timer_NeedsUpdate;
        using HandleType = FCk_Handle_Timer;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
