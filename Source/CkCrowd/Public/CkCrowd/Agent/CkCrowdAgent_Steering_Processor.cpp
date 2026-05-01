#include "CkCrowdAgent_Steering_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_Steering);

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
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
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

        // Past the final waypoint → goal reached. Sub-task 2E will fire OnGoalReached here once that
        // signal exists; for now we just stop.
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

        // Final-stop tolerance: if we're within ArrivalRadius of the very last waypoint AND that's the
        // current target, brake hard.
        const auto IsTargetingFinal = (InPathFollow._WaypointIndex == Waypoints.Num() - 1);
        if (IsTargetingFinal && DistanceToNext <= InParams.Get_ArrivalRadius())
        {
            Bail();
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();

        // Braking ramp: stopping distance for speed v at deceleration a is v² / (2a). Solving for v
        // given the remaining distance gives the max speed at which we can still stop in time.
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        const auto TargetSpeed = FMath::Min(MaxSpeed, BrakingSpeedCap);

        // Acceleration clamp from last frame's desired speed. This is intentionally NOT reading
        // FFragment_Velocity_Current — per CkCrowd anti-patterns, steering decisions read the
        // steering output (decoupled from the velocity-clamp processor's trimming).
        const auto PreviousSpeed = InDesired._Velocity.Size();
        const auto SpeedDelta = MaxAccel * InDeltaT.Get_Seconds();
        const auto NewSpeed = FMath::Clamp(
            TargetSpeed,
            FMath::Max(0.0f, PreviousSpeed - SpeedDelta),
            PreviousSpeed + SpeedDelta);

        InDesired._Velocity = Direction * NewSpeed;
    }
}

// --------------------------------------------------------------------------------------------------------------------
