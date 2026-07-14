#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Bridge: PathNetwork corridor → CkNavigation path-result seam. Watches agents that carry a
    // follower corridor (i.e. agents whose MoveTo was routed through the path network by
    // HandleRequests) and installs resolved corridors via FCk_Nav_Algorithm::InstallExternalPath —
    // from that point the existing OnPathResolved poll and steering machinery take over, unable to
    // tell a corridor from a navmesh path.
    //
    // The view deliberately does NOT require PathPending: a network rebuild replans corridors for
    // agents that are already WALKING (FProcessor_PathNetworkFollower_InvalidateOnRebuild), and the
    // fresh corridor must swap in mid-walk. FFragment_CrowdAgent_InstalledRoute (goal + network
    // epoch) is the install-identity that separates "fresh plan to consume" from "corridor I
    // already installed" in that steady state.
    //
    // Corridor Failed while PathPending → Idle + OnGoalFailed, mirroring OnPathResolved's failure
    // branch. Failed while WALKING (rebuild replan came back unroutable) keeps the agent on its
    // already-installed waypoints — they are world-space points and remain walkable; degrading to
    // a hard stop mid-street would be worse.
    //
    // PumpPolicy::SkipPump — the body broadcasts OnGoalFailed and Nav_OnPathReady (via the install
    // seam); pumping with DeltaT=0 would re-broadcast before the tag transition takes effect.
    class CKCROWD_API FProcessor_CrowdAgent_OnRouteResolved : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_OnRouteResolved,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_PathNetworkFollower_Corridor>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        // HandleRequests stamps PathPending + _ActiveGoal + parks the nav-path slot that this
        // processor keys off; RunAfter it so a MoveTo issued this frame is observed with
        // consistent state.
        using RunAfter = TDepList<FProcessor_CrowdAgent_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
