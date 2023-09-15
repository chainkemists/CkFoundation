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
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKTIMER_API FProcessor_Timer_Update
        : public TProcessor<FProcessor_Timer_Update, FFragment_Timer_Params, FFragment_Timer_Current, FTag_Timer_NeedsUpdate>
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

    class CKTIMER_API FProcessor_Timer_Replicate
        : public TProcessor<FProcessor_Timer_Replicate,
            FFragment_Timer_Current, TObjectPtr<UCk_Fragment_Timer_Rep>, FTag_Timer_Updated>
    {
    public:
        using MarkedDirtyBy = FTag_Timer_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Timer_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Timer_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
