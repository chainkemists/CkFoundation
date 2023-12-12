#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkTimer/CkTimer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKTIMER_API FProcessor_Timer_HandleRequests
        : public TProcessor<FProcessor_Timer_HandleRequests, FFragment_Timer_Current, FFragment_Timer_Params, FFragment_Timer_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_Timer_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            FFragment_Timer_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Manipulate& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Jump& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            FFragment_Timer_Current& InCurrentComp,
            const FFragment_Timer_Params& InParamsComp,
            const FCk_Request_Timer_Consume& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_Update
        : public TProcessor<FProcessor_Timer_Update, FFragment_Timer_Params, FFragment_Timer_Current, FTag_Timer_NeedsUpdate,
            ck::TExclude<FTag_Timer_Countdown>>
    {
    public:
        using MarkedDirtyBy = FTag_Timer_NeedsUpdate;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_Update_Countdown
        : public TProcessor<FProcessor_Timer_Update_Countdown, FFragment_Timer_Params, FFragment_Timer_Current, FTag_Timer_NeedsUpdate,
            FTag_Timer_Countdown>
    {
    public:
        using MarkedDirtyBy = FTag_Timer_NeedsUpdate;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InTimerEntity,
            const FFragment_Timer_Params& InParams,
            FFragment_Timer_Current& InCurrentComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
