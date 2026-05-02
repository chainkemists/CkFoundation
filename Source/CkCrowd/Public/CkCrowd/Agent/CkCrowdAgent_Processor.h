#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gate 0+3: consumes FTag_CrowdAgent_NeedsSetup. Spawns the agent's probe child entity
    // (Cylinder + Probe filtered on Crowd.Agent), SceneNode-parents it to the agent so it tracks
    // the agent's apply-offset moves, stores the probe handle on the agent via FFragment_CrowdAgent_ProbeRef,
    // and stamps FTag_CrowdAgent_HasProbe + clears the NeedsSetup tag so the body fires exactly once.
    //
    // The view requires FFragment_Transform — set up depends on the agent already having a Transform
    // because the SceneNode parent is the agent's Transform, and we offset the probe child by HalfHeight
    // along Z (agent location is its FEET, probe shape is centered). The gym pattern adds Transform
    // immediately after utils_crowd_agent::Add, so this view-membership wait is a single-tick latency
    // at most — and it's a hard error if Transform is never added (no probe → no separation).
    //
    // TExclude<FTag_CrowdAgent_HasProbe> prevents re-entry. The Setup body manually removes
    // FTag_CrowdAgent_NeedsSetup at the end, mirroring the CkPmg DebugShape Setup pattern.
    class CKCROWD_API FProcessor_CrowdAgent_Setup : public ck_exp::TProcessor<
        FProcessor_CrowdAgent_Setup,
        FCk_Handle_CrowdAgent,
        ck::TReadOnly<FFragment_CrowdAgent_Params>,
        ck::TReadOnly<FFragment_Transform>,
        ck::TReadWrite<FFragment_CrowdAgent_ProbeRef>,
        FTag_CrowdAgent_NeedsSetup,
        TExclude<FTag_CrowdAgent_HasProbe>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_CrowdAgent_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Transform& InTransform,
            FFragment_CrowdAgent_ProbeRef& InProbeRef) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
