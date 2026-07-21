#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The stage the dtCrowd port originally dropped: Detour passes every integrated agent position
    // through dtPathCorridor::movePosition (DetourCrowd.cpp:1345, updateStepMove) — a
    // moveAlongSurface walk that constrains the agent to the polygon mesh, so a dtCrowd agent
    // CANNOT leave the navmesh; walls stop it and it slides. Without this stage, any lateral force
    // (separation redirect, avoidance velocity, push-apart shove) moves the transform straight
    // through a navmesh boundary — the wall-eroded band around geometry included.
    //
    // ApplyOffset (integrator delta) and PushApart (de-overlap shove) both stage into
    // FFragment_CrowdAgent_PendingDisplacement; this processor is the SINGLE Transform writer for
    // a crowd agent. It walks the accumulated displacement along the mesh via
    // ANavigationData::FindMoveAlongSurface and enqueues one Request_AddLocationOffset. XY is
    // constrained; Z stays owned by path-follow and the integrator.
    //
    // Worlds with no nav data pass displacements through untouched (nav-less tests / gameplay).
    // An agent found off-mesh while nav data exists is snapped back if a wider search finds the
    // mesh (self-healing — a one-frame corner leak must not disable the clamp forever).
    //
    // Group: FGroup_Physics. RunAfter PushApart (the last staging writer). The resulting
    // AddLocationOffset request is drained by Transform_HandleRequests.
    class CKCROWD_API FProcessor_CrowdAgent_ConstrainToNavmesh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ConstrainToNavmesh,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PendingDisplacement& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
