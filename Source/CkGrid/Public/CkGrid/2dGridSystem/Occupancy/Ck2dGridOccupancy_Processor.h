#pragma once

#include "Ck2dGridOccupancy_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Deliberately un-gated: a destroyed placement prunes the record via CkRecord's reverse-link
    // whenever that lands, so no dirty tag can be relied on to re-trigger the un-stamp. The diff
    // makes an unchanged grid a no-op.
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

    // Clients NEVER originate placements — this only mirrors replicated state.
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
