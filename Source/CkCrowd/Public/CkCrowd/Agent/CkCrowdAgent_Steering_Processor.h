#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Path follower: advances the waypoint cursor and writes _DesiredVelocity (direction to the next
    // waypoint, ramp clamped by _MaxAcceleration, braking keyed off distance to the FINAL waypoint).
    // RunBefore EulerIntegrator_Update so the velocity-bridge writes Velocity_Current first.
    class CKCROWD_API FProcessor_CrowdAgent_Steering : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_Steering,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_SeparationForce>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunBefore = TDepList<FProcessor_EulerIntegrator_Update>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;

    public:
        // The link half of the waypoint cursor. Public because the GroundNav install seam drives the
        // same two pieces the cursor does — a fresh route stamps the spans and abandons the crossing
        // the replaced route was on — and two copies of what a crossing means would drift.

        // What the ground route about to be walked crosses, in walk order. A route no link put a
        // waypoint on stamps an empty array, which is also what clears the previous route's spans.
        static auto
        DoStampLinkSpans(
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FCk_GroundNavPath_Result&  InResult) -> void;

        // The crossing's identity on the neutral handshake, derived from the route and the link rather
        // than minted, so the Complete that ends one names what the Begin that started it named
        // without either side storing it anywhere but the agent.
        static auto
        Get_LinkTraversalCorrelator(
            const FFragment_CrowdAgent_PathFollow& InPathFollow,
            int32                                  InLinkId) -> int32;

        // Ends the recorded crossing as Failed_Cancelled and forgets it. Every route drop calls this:
        // the body is no longer on the waypoints that bounded the crossing.
        static auto
        DoCancelActiveLinkTraversal(
            FCk_Handle&                      InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;

        // Reconciles the recorded crossing against where the cursor now stands: Begin on the span the
        // cursor has reached and not passed, Complete once it is past that span's exit, Cancel when
        // the route ran out under a crossing that never finished.
        static auto
        DoDriveLinkTraversalCursor(
            FCk_Handle&                      InHandle,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            int32                            InCursor,
            int32                            InWaypointCount) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
