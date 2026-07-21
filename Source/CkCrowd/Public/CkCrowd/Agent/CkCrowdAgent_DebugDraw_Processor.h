#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Separation_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gym-side visual diagnostics. Draws three things per agent every frame, gated by
    // the `ck.Crowd.Debug` CVar:
    //   - Yellow circle on the ground at _SeparationRadius (the falloff cutoff)
    //   - Orange arrow from agent center along the separation force direction, scaled by magnitude
    //   - Cyan lines from agent center to each neighbor center
    //
    // Group: FGroup_Physics, RunAfter Separation so the force we draw is the one the steering
    // layer will actually use. Excludes asleep agents — no point drawing for entities that aren't
    // contributing to the simulation.
    //
    // Defaults to off; flip on in PIE with `ck.Crowd.Debug 1`. PMG-style entity-based overlays
    // would carry per-tick allocation cost; immediate-mode DrawDebug calls are the right tool
    // for "always-current, never-persistent" visualization.
    class CKCROWD_API FProcessor_CrowdAgent_DebugDraw : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DebugDraw,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadOnly<FFragment_CrowdAgent_SeparationForce>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Separation>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
