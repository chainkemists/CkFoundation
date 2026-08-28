#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // dtCrowd-style penalty-scored velocity sampling; overwrites _DesiredVelocity with the
    // lowest-penalty candidate. PARALLEL-SAFE: every cross-entity access is a read (the zone-tag
    // walk, the navmesh wall query, and the final-approach envelope's settled-neighbour scan), and
    // the only writes are the agent's own DesiredVelocity and its own LocalBoundary cache. The
    // settled scan reads state written by BlockDetect and Steering, both of which this processor
    // already runs after within FGroup_Physics, and by tiers that live in FGroup_Gameplay — so no
    // writer of it is ever concurrent with this scan. The navmesh query is worker-thread safe by
    // construction:
    // ARecastNavMesh reaches Detour through INITIALIZE_NAVQUERY, which uses a STACK-LOCAL
    // dtNavMeshQuery off the game thread and the shared one only on it (RecastNavMesh.cpp:49-52),
    // and the dtNavMesh itself is only read.
    //
    // A flying agent is excluded: the velocity-obstacle quadratic, its penalties and its side
    // preference are all FVector2D, so sampling would flatten a 3D route's desired velocity onto the
    // XY plane. Flyers get no avoidance until a volumetric sampler exists.
    class CKCROWD_API FProcessor_CrowdAgent_AvoidanceSample : public TParallelProcessor<
            FProcessor_CrowdAgent_AvoidanceSample,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            TReadOnly<FFragment_Transform>,
            TReadOnly<FFragment_CrowdAgent_Params>,
            TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            TReadOnly<FFragment_CrowdAgent_AvoidanceVolumeCache>,
            TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TReadWrite<FFragment_CrowdAgent_LocalBoundary>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering, FProcessor_CrowdAgent_NeighborSync>;

    public:
        using TParallelProcessor::TParallelProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_AvoidanceVolumeCache& InAvoidanceVolumeCache,
            FFragment_CrowdAgent_DesiredVelocity& InDesired,
            FFragment_CrowdAgent_LocalBoundary& InBoundary) const -> void;

    private:
        static auto
        DoRefreshLocalBoundary(
            const FCk_Handle& InAgent,
            const FVector& InAgentLocation,
            float InQueryRange,
            const FVector& InProjectionExtent,
            FFragment_CrowdAgent_LocalBoundary& InOutBoundary) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
