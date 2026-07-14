#include "CkCrowdAgent_Steering_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Steering);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::Steering"), STAT_CkCrowd_SteeringProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_Steering::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_SteeringProc);

        const auto Bail = [&]()
        {
            InDesired._Velocity = FVector::ZeroVector;
        };

        if (InPathResult.Get_Status() != ECk_Nav_PathStatus::Ready &&
            InPathResult.Get_Status() != ECk_Nav_PathStatus::Partial)
        {
            Bail();
            return;
        }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        {
            Bail();
            return;
        }

        // Reading the current world location requires the agent to have a Transform feature; the gym /
        // game code is responsible for adding it before issuing a path. If absent, just zero the desired
        // velocity rather than ensure-spamming — the steering loop is per-frame.
        auto TransformHandle = UCk_Utils_Transform_UE::CastChecked(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        {
            Bail();
            return;
        }
        const auto CurrentLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        // Advance the waypoint cursor past every INTERMEDIATE waypoint that is either (a) inside the
        // arrival radius, or (b) already BEHIND the agent relative to the incoming path segment.
        //
        // (b) mirrors UPathFollowingComponent::HasReachedCurrentTarget's primary retirement test
        // (PathFollowingComponent.cpp:1293-1302): dot(target - feet, segmentDir) < 0 => passed it.
        // Proximity ALONE cannot retire a waypoint the agent's turn-limited arc misses: the minimum
        // turning radius is MaxSpeed / MaxTurnRate (60cm at defaults), which structurally EXCEEDS
        // _WaypointArrivalRadius (25cm). Any deflection near a waypoint — an avoidance manoeuvre, a
        // separation shove — could leave the cursor pinned on a waypoint the agent had physically
        // driven past, whereupon path-follow aimed BACKWARD at it, straight through whoever pushed
        // it there. UE's proximity leg is a mere 5% of agent radius; the plane test does the work.
        //
        // The FINAL waypoint is deliberately NOT retired here (note the Num() - 1 bound): goal
        // arrival belongs exclusively to the final-stop branch below, which broadcasts OnGoalReached
        // and flips Walking -> Idle. The old loop could silently consume a final waypoint that sat
        // inside the arrival radius, leaving the cursor past the end with no signal ever fired — a
        // latent liveness bug (agent stuck Walking at zero velocity, goal never reported).
        const auto WaypointArrivalRadius = InParams.Get_WaypointArrivalRadius();
        while (InPathFollow._WaypointIndex < Waypoints.Num() - 1)
        {
            const auto& Waypoint = Waypoints[InPathFollow._WaypointIndex];

            const auto WithinArrivalRadius =
                FVector::Dist(CurrentLoc, Waypoint) <= WaypointArrivalRadius;

            const auto CrossedWaypointPlane = [&]() -> bool
            {
                const auto SegmentDir = (Waypoint - InPathFollow.Get_CurrentSegmentStart()).GetSafeNormal();

                // Degenerate segment (duplicate waypoint, or an install that could not capture the
                // agent's location): fall back to proximity alone rather than trust a zero normal.
                if (SegmentDir.IsNearlyZero())
                { return false; }

                return FVector::DotProduct(Waypoint - CurrentLoc, SegmentDir) < 0.0;
            }();

            if (NOT (WithinArrivalRadius || CrossedWaypointPlane))
            { break; }

            InPathFollow._CurrentSegmentStart = Waypoint;
            ++InPathFollow._WaypointIndex;
        }

        // Defensive bail: if cursor somehow advanced past Num before the final-stop branch fires
        // (e.g. a sufficiently large per-frame offset crossing the entire path tail at once), just
        // stop. The arrival side effects (broadcast + tag transition) are handled in the final-stop
        // branch below.
        if (InPathFollow._WaypointIndex >= Waypoints.Num())
        {
            Bail();
            return;
        }

        const auto& TargetLoc = Waypoints[InPathFollow._WaypointIndex];
        const auto ToTarget = TargetLoc - CurrentLoc;
        const auto DistanceToNext = ToTarget.Size();
        if (DistanceToNext <= KINDA_SMALL_NUMBER)
        {
            Bail();
            return;
        }
        const auto Direction = ToTarget / DistanceToNext;

        // Distance from current position along the polyline to the FINAL waypoint. Used for braking:
        // a bird's-eye distance to the final point would under-estimate when the path bends, causing
        // the agent to overshoot. Summing segment lengths from the current waypoint onward is a tight
        // upper bound (the Recast funnel guarantees waypoints are LOS-connected, so segments are real).
        auto DistanceToFinal = DistanceToNext;
        for (auto i = InPathFollow._WaypointIndex; i < Waypoints.Num() - 1; ++i)
        {
            DistanceToFinal += FVector::Dist(Waypoints[i], Waypoints[i + 1]);
        }

        // Final-stop tolerance: within _ActiveArrivalRadius of the very last waypoint AND that's the
        // current target → goal reached. Broadcast OnGoalReached and transition Walking → Idle so
        // steering's view filter stops firing on this entity. The bridge keeps writing zero velocity
        // while Idle, and the agent rests in place. _ActiveArrivalRadius was cached by HandleRequests
        // from either the params default or the per-MoveTo override.
        const auto IsTargetingFinal = (InPathFollow._WaypointIndex == Waypoints.Num() - 1);
        if (IsTargetingFinal && DistanceToNext <= InPathFollow.Get_ActiveArrivalRadius())
        {
            InPathFollow._WaypointIndex = Waypoints.Num();
            Bail();

            auto NonConstHandle = InHandle;
            NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
            NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();

            UUtils_Signal_CrowdAgent_OnGoalReached::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle));
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxTurnRate = InParams.Get_MaxTurnRate();

        // Braking ramp: stopping distance for speed v at deceleration a is v² / (2a). Solving for v
        // given the remaining distance gives the max speed at which we can still stop in time.
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        // Turn-radius arrival cap: an agent moving at speed v can turn no tighter than its minimum
        // turning radius r = v / MaxTurnRate (the velocity direction is bounded to MaxTurnRate rad/s
        // by AccelClamp). If r exceeds the remaining distance to the goal, the agent physically
        // cannot curve onto the goal and ORBITS it instead — the root cause of the "agent circles
        // its target" bug. Capping speed at v <= MaxTurnRate * DistanceToFinal keeps the turning
        // radius within the remaining distance, so the agent spirals in and stops. It only bites in
        // the final ~MaxSpeed/MaxTurnRate cm (≈60cm at defaults); on open path it has no effect.
        auto TurnRadiusSpeedCap = MaxSpeed;
        if (MaxTurnRate > 0.0f)
        {
            TurnRadiusSpeedCap = MaxTurnRate * DistanceToFinal;
        }

        // Phase 1.2 — the per-frame scalar PreviousSpeed clamp that used to live here is gone;
        // FProcessor_CrowdAgent_AccelClamp now ramps the velocity in vector space (so direction
        // changes are bounded too, not just magnitude). Steering writes the raw target velocity;
        // AccelClamp downstream brings it into the per-frame budget.
        const auto NewSpeed = FMath::Min3(MaxSpeed, BrakingSpeedCap, TurnRadiusSpeedCap);

        // Gate 3C (revised) — combine path-follow with separation, dtCrowd-style.
        //
        // The previous combination damped path-follow by separation intensity
        // (PathFollowDamp = max(0, 1 - SepMag/MaxSpeed)). Head-on separation is ANTIPARALLEL to the
        // path direction, so the two terms cancelled and the desired velocity collapsed toward zero
        // exactly when neighbours pressed. That zero is a fixed point, and it is load-bearing for a
        // freeze: FProcessor_CrowdAgent_AvoidanceSample runs after this and anchors its penalty
        // scoring on the value written here, so a ~zero anchor makes "stay stopped" the cheapest
        // candidate — two agents converging on a shared waypoint stop dead and never resolve.
        //
        // dtCrowd never yields the desired velocity: updateStepSteering rebuilds dvel at full
        // maxSpeed every frame (DetourCrowd.cpp:1468-1469), and where a separation contribution
        // opposes it, that contribution is clamped to PURE LATERAL (DetourCrowd.cpp:1499-1510).
        // Separation may REDIRECT forward motion; it may never CANCEL it. Braking belongs to the
        // goal ramps above, and stopping belongs to Request_Stop / the final-stop latch — not to a
        // neighbour pressing on us.
        //
        // Aggregate form of that per-neighbour clamp: the component of the accumulated separation
        // force pushing WITH the path passes through untouched; a component pushing AGAINST it is
        // redirected to whichever side the force already leans toward (the Separation solver's
        // deterministic per-agent jitter guarantees a lean in all but pathological cases). A
        // perfectly mirrored head-on pair falls back to each agent's own LEFT — matching the
        // sampler's PassLeft default — and two agents facing each other have opposite world lefts,
        // so they diverge rather than mirror.
        const auto& RawSeparation = InSeparationForce.Get_Force();
        const auto SeparationAlongPath = FVector::DotProduct(RawSeparation, Direction);

        auto SeparationVec = RawSeparation;
        if (SeparationAlongPath < 0.0)
        {
            const auto LateralComponent = RawSeparation - (SeparationAlongPath * Direction);
            const auto SideDir = LateralComponent.GetSafeNormal();

            if (NOT SideDir.IsNearlyZero())
            {
                SeparationVec = LateralComponent + SideDir * (-SeparationAlongPath);
            }
            else
            {
                const auto LeftPerp = FVector{-Direction.Y, Direction.X, 0.0}.GetSafeNormal();
                if (NOT LeftPerp.IsNearlyZero())
                {
                    SeparationVec = LeftPerp * (-SeparationAlongPath);
                }
                // else: Direction is near-vertical — degenerate for a walking agent. Leave the raw
                // force untouched rather than invent an axis.
            }
        }

        const auto Combined = Direction * NewSpeed + SeparationVec;
        InDesired._Velocity = Combined.GetClampedToMaxSize(MaxSpeed);
    }
}

// --------------------------------------------------------------------------------------------------------------------
