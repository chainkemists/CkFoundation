#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_OnGroundNavPathResolved_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The view deliberately does NOT require PathPending: a network rebuild replans corridors for
    // agents that are already WALKING, and the fresh corridor must swap in mid-walk.
    //
    // PumpPolicy::SkipPump — the body broadcasts OnGoalFailed and Nav_OnPathReady (via the install
    // seam); pumping with DeltaT=0 would re-broadcast before the tag transition takes effect.
    class CKCROWD_API FProcessor_CrowdAgent_OnRouteResolved : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_OnRouteResolved,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_PathNetworkFollower_Corridor>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_PathTrouble>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        // HandleRequests stamps PathPending + _ActiveGoal + parks the nav-path slot this processor
        // keys off, so a MoveTo issued this frame is observed with consistent state.
        // Every resolve processor installs into FFragment_CrowdAgent_PathFollow; the chain fixes which install wins.
        using RunAfter = TDepList<
            FProcessor_CrowdAgent_HandleRequests,
            FProcessor_CrowdAgent_OnGroundNavPathResolved>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_PathTrouble& InPathTrouble) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
