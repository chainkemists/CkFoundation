#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drains FFragment_Nav_Requests for entities that have one. Calls FindPathSync per
    // request, populates FFragment_Nav_PathResult, fires Nav_OnPathReady or
    // Nav_OnPathFailed signal.
    //
    // Budget: respects UCk_Nav_ProjectSettings_UE::Get_MaxPathQueriesPerFrame() across
    // the entire processor pass. If exceeded, the remaining requests are dropped this
    // frame (their PathResult.Diagnostics records BudgetExceeded; the request fragment
    // is rebuilt with the un-drained requests for next-tick retry).
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
        // Mutable per-tick budget cursor — reset in DoTick, decremented per request drained.
        mutable int32 _BudgetRemainingThisTick = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
