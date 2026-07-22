#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVisibleRange/CkVisibleRange_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Runs every tick: per-entity cadence cannot use TProcessorBase's _TickRate (that throttles the whole
    // processor to one uniform rate), so each entity gates its own re-evaluation via its Chrono ticked Wrap.
    class CKVISIBLERANGE_API FProcessor_VisibleRange_Update : public ck_exp::TProcessor<
        FProcessor_VisibleRange_Update,
        FCk_Handle_VisibleRange,
        ck::TReadOnly<FFragment_VisibleRange_Params>,
        ck::TReadWrite<FFragment_VisibleRange_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisibleRange_Params& InParams,
            FFragment_VisibleRange_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISIBLERANGE_API FProcessor_VisibleRange_HandleRequests
        : public ck_exp::TProcessor<FProcessor_VisibleRange_HandleRequests, FCk_Handle_VisibleRange,
            ck::TReadWrite<FFragment_VisibleRange_Current>, ck::TReadWrite<FFragment_VisibleRange_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VisibleRange_Update>;
        using MarkedDirtyBy = FFragment_VisibleRange_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            FFragment_VisibleRange_Requests& InRequests) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_ApplyRangeState& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_SetVisibility& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
