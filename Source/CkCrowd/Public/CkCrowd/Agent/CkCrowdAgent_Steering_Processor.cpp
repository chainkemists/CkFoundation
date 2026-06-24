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

        // Advance the waypoint cursor past every waypoint we've already crossed (handles cases where a
        // single frame's offset crossed multiple closely-spaced waypoints — a rare-but-possible burst).
        const auto WaypointArrivalRadius = InParams.Get_WaypointArrivalRadius();
        while (InPathFollow._WaypointIndex < Waypoints.Num() &&
               FVector::Dist(CurrentLoc, Waypoints[InPathFollow._WaypointIndex]) <= WaypointArrivalRadius)
        {
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

        // Gate 3C — combine path-follow with separation. Naive `path + separation` produces
        // vibration on head-on encounters: both forces fire at full strength, the clamp eats both,
        // agents wobble through each other while neither concedes. Damp path-follow by the
        // separation intensity so the two forces cooperate — when neighbors are pressing hard,
        // the agent gives way; as the neighbor recedes, path-follow ramps back to full.
        //
        // Damping target is MaxSpeed (not the per-frame braking-ramp speed). Path-follow goes to
        // zero exactly when separation magnitude reaches MaxSpeed, so the clamp never has to
        // truncate. Above that, separation alone drives the velocity (clamped at the end as a
        // safety net for many-neighbor pile-ups). At goal, path-follow has already braked to zero
        // via the final-stop branch, so separation alone keeps doing its job for the cluster.
        const auto& SeparationVec = InSeparationForce.Get_Force();
        const auto SeparationMag = SeparationVec.Size();
        const auto PathFollowDamp = FMath::Max(0.0, 1.0 - (SeparationMag / MaxSpeed));
        const auto PathFollowVelocity = Direction * NewSpeed * PathFollowDamp;
        const auto Combined = PathFollowVelocity + SeparationVec;
        InDesired._Velocity = Combined.GetClampedToMaxSize(MaxSpeed);
    }
}

// --------------------------------------------------------------------------------------------------------------------
