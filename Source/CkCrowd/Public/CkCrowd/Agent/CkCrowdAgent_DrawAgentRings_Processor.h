#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-frame "orbit diagnosis" overlay for the debugger-selected agent (or every agent when
    // `ck.Crowd.DrawAgentRings` is on). Draws, in world space:
    //   - the arrival ring (green) at the active goal, radius = the goal's active arrival tolerance,
    //   - the predicted-orbit ring (red) at the active goal, radius = MaxSpeed / MaxTurnRate — the
    //     orbit the agent settles into at full speed if its turn radius can't fit the stop-ring,
    //   - the turn-radius circle (blue) tangent to the agent, centre perpendicular-left of velocity,
    //     radius = currentSpeed / MaxTurnRate — the tightest arc it can physically make right now,
    //   - the velocity vector (yellow arrow).
    //
    // These are the in-world counterparts of the CkCrowd Debugger's per-agent diagnostics. The
    // arrival/orbit rings only draw while the agent is Walking (a stale goal on an idle agent would
    // be misleading); the turn-radius circle + velocity draw whenever the agent is moving.
    //
    // Gating mirrors the planned-path/breadcrumb processors: draw if `ck.Crowd.DrawAgentRings` is on
    // OR the agent matches `ck.Crowd.SelectedEntityId`.
    class CKCROWD_API FProcessor_CrowdAgent_DrawAgentRings : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawAgentRings,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
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
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
