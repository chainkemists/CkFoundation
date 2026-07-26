#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNAVIGATION_API FProcessor_Nav_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Nav_HandleRequests,
        FCk_Handle,
        ck::TReadWrite<FFragment_Nav_Requests>,
        ck::TReadWrite<FFragment_Nav_PathResult>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Nav_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Nav_Requests& InRequests,
            FFragment_Nav_PathResult& InResult) const -> void;

    private:
        // Reset in DoTick but never enforced — the per-tick query cap is not wired up.
        mutable int32 _BudgetRemainingThisTick = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
