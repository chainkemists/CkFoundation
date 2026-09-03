#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The flying counterpart of FProcessor_CrowdAgent_ConstrainToNavmesh, and the reason excluding a
    // flying agent from that processor does not simply freeze it: FFragment_CrowdAgent_PendingDisplacement
    // is a staging slot that ONE processor has to drain, and the navmesh constraint was the only drain.
    //
    // A flying agent has no surface to be walked along, so this is the constraint's own
    // no-nav-data branch standing on its own: consume the accumulated displacement and enqueue it
    // whole, all three components, with no projection and no surface height substituted for Z.
    //
    // Requires FTag_CrowdAgent_Flying, which ConstrainToNavmesh excludes, so the two views partition
    // the agent population and the module keeps exactly one Transform-translation writer per agent.
    // Group and RunAfter are the constraint's verbatim — PushApart is the last staging writer, and
    // it excludes flying agents too, so for these the staged value is the integrator delta alone.
    class CKCROWD_API FProcessor_CrowdAgent_ApplyDisplacement3D : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_ApplyDisplacement3D,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Flying,
            ck::TReadWrite<FFragment_CrowdAgent_PendingDisplacement>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart>;
        // Both drain FFragment_CrowdAgent_PendingDisplacement. The views partition on Flying so the
        // choice is free, but it has to be declared rather than fall out of registration order.
        using RunBefore = TDepList<FProcessor_CrowdAgent_ConstrainToNavmesh>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_PendingDisplacement& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
