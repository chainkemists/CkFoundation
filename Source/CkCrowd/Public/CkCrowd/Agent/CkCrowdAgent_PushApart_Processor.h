#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_ApplyOffset_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stages into FFragment_CrowdAgent_PendingDisplacement and never writes the Transform — the
    // single Transform writer is FProcessor_CrowdAgent_ConstrainToNavmesh.
    // No FTag_CrowdAgent_Walking requirement, deliberately: ordinary idle agents must separate
    // too. GoalFailedHold is the sole exception: it yields zero while a non-held pair member
    // absorbs the full correction, preserving a physically stable terminal hold.
    //
    // A flying agent is excluded: the shove is zeroed in Z by construction, so two agents stacked
    // vertically would be de-overlapped sideways.
    class CKCROWD_API FProcessor_CrowdAgent_PushApart : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PushApart,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            // A permeable agent does not de-overlap: bodies interpenetrate on purpose. Safe as a
            // view exclusion (unlike Separation) because this processor STAGES into
            // PendingDisplacement, which ConstrainToNavmesh consumes and clears every frame -- a
            // skipped agent leaves nothing stale behind.
            TExclude<FTag_CrowdAgent_Permeable>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_ApplyOffset>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_PendingDisplacement& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
