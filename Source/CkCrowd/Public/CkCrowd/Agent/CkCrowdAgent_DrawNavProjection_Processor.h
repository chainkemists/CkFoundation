#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Always-on overlay: for each crowd agent, projects its current world position onto the
    // navmesh and draws a small circle at the projection point — green if the projection
    // succeeded (the agent is somewhere reachable), red if it failed (the agent is in a void
    // the nav system can't snap to). Cheap, immediate-mode draw, no CVar gating.
    //
    // Useful as a primary diagnostic: an agent that's "floating" usually has its root transform
    // off the navmesh entirely, and a red circle (or no circle) immediately tells the dev that
    // the navmesh doesn't cover where the agent ended up.
    class CKCROWD_API FProcessor_CrowdAgent_DrawNavProjection : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawNavProjection,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
