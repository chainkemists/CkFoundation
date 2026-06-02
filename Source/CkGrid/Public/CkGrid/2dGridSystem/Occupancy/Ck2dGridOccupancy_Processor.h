#pragma once

#include "Ck2dGridOccupancy_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Runs on every grid that owns an occupancy record. Diffs the record-derived desired
    // occupancy against the currently-stamped set and stamps / un-stamps cells accordingly.
    // Un-gated (runs every tick): a placement's destruction prunes the record via CkRecord's
    // reverse-link, and the next pass un-stamps its cells — correct regardless of when the
    // pruning lands, without depending on a re-triggered dirty tag. The diff makes an
    // unchanged grid a no-op. (Candidate for dirty-gating later if profiling warrants.)
    class CKGRID_API FProcessor_2dGridOccupancy_StampCells : public ck_exp::TProcessor<
            FProcessor_2dGridOccupancy_StampCells,
            FCk_Handle_2dGridSystem,
            TReadWrite<FFragment_2dGridOccupancy_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_2dGridOccupancy_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
