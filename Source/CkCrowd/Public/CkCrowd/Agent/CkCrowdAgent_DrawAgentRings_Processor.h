#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-frame orbit-diagnosis overlay: arrival ring (green) and predicted-orbit ring
    // (MaxSpeed / MaxTurnRate, red) at the active goal while Walking, plus the current turn-radius
    // circle (blue) and the velocity arrow (yellow) while moving.
    // Draws when `ck.Crowd.DrawAgentRings` is on OR the agent is `ck.Crowd.SelectedEntityId`.
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
