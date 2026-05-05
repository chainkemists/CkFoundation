#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-frame planned-path overlay. Reads FFragment_Nav_PathResult (the waypoints CkNavigation
    // resolved for the agent's current goal) and draws them as a polyline at agent body height.
    // Per-agent colour comes from UCk_Utils_CrowdAgent_UE::Get_DebugColor — coordinated with the
    // breadcrumb processor so planned-vs-actual visually link to the same agent identity.
    //
    // Gating mirrors the breadcrumb processor: draws if `ck.Crowd.DrawPlannedPaths` is on OR the
    // agent matches `ck.Crowd.SelectedEntityId`. Only crowd agents that have a path result fragment
    // satisfy the view (i.e. agents that have been issued at least one MoveTo).
    class CKCROWD_API FProcessor_CrowdAgent_DrawPlannedPath : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawPlannedPath,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Nav_PathResult& InPathResult) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
