#pragma once

#include "Ck2dGridOccupancy_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
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

    // --------------------------------------------------------------------------------------------------------------------

    // AuthorityOnly. Gated on the MayRequireReplication tag, set whenever the placement record
    // mutates. Serializes the live record's valid placements into FCk_RepData_2dGridPlacements and
    // pushes it through the container entry ref (mirrors FProcessor_Inventory_Spatial_Replicate).
    class CKGRID_API FProcessor_2dGridOccupancy_Replicate : public ck_exp::TProcessor<
            FProcessor_2dGridOccupancy_Replicate,
            FCk_Handle_2dGridSystem,
            FTag_2dGridOccupancy_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_2dGridOccupancy_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives a RESTORED occupancy grid after a snapshot load. The placement record + each
    // placement's Params round-trip; everything else re-derives: this processor re-adds the
    // (derived) FFragment_2dGridOccupancy_Current so the StampCells reconcile rebuilds the stamped
    // cells from the restored record, re-creates the grid-resident replicated container (Construct
    // is abstained during reconstitution), and re-arms FTag_2dGridOccupancy_MayRequireReplication so
    // the authority Replicate pass rebuilds the wire entries — fresh post-travel clients then
    // converge through the ordinary SyncReplication path. Waits for the grid's live half
    // (FProcessor_2dGridSystem_RestoreRecompose) and, for replicated grids, the respawn pass's
    // re-established driver. Restored placement ENTITIES need nothing else: they are server-local
    // (clients rebuild their own from the wire entries). The view iterates the restored placement
    // record and POINT-QUERIES the shared restored marker, paired with the per-feature done tag.
    class CKGRID_API FProcessor_2dGridOccupancy_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_2dGridOccupancy_ReplicateOnRestore,
            FCk_Handle,
            TReadOnly<FFragment_RecordOf_GridPlacements>,
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
            const FFragment_RecordOf_GridPlacements& InRecord) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ClientOnly. Gated on the SyncReplication fragment the RegisterLazy handler stamps. Diffs the
    // current vs previous replicated entries (by occupant identity) and rebuilds / tears down client
    // placement entities; the un-gated StampCells pass then mirrors the cells (mirrors
    // FProcessor_Inventory_Spatial_SyncReplication). Clients NEVER create placements themselves.
    class CKGRID_API FProcessor_2dGridOccupancy_SyncReplication : public ck_exp::TProcessor<
            FProcessor_2dGridOccupancy_SyncReplication,
            FCk_Handle_2dGridSystem,
            TReadOnly<FFragment_2dGridOccupancy_SyncReplication>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FFragment_2dGridOccupancy_SyncReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridOccupancy_SyncReplication& InSync) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
