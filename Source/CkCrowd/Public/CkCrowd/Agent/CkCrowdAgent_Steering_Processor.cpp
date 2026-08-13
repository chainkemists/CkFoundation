#include "CkCrowdAgent_Steering_Processor.h"

#include "CkCrowd/CkCrowd_Log.h"

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
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            const FFragment_Nav_PathResult& InPathResult,
            const FFragment_CrowdAgent_SeparationForce& InSeparationForce,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_SteeringProc);

        const auto DoZeroDesiredVelocity = [&]()
        {
            InDesired._Velocity = FVector::ZeroVector;
        };

        if (InPathResult.Get_Status() != ECk_Nav_PathStatus::Ready &&
            InPathResult.Get_Status() != ECk_Nav_PathStatus::Partial)
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.Num() == 0)
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto CurrentLoc = InTransform.Get_Transform().GetLocation();

        // The plane test does the real work: the minimum turning radius (MaxSpeed / MaxTurnRate,
        // 60cm at defaults) structurally EXCEEDS _WaypointArrivalRadius (25cm), so proximity alone
        // cannot retire a waypoint the agent's turn-limited arc missed and path-follow would then
        // aim BACKWARD at it. The FINAL waypoint is deliberately never retired here (Num() - 1
        // bound) — goal arrival belongs to the final-stop branch, which is what fires OnGoalReached.
        const auto WaypointArrivalRadius = InParams.Get_WaypointArrivalRadius();
        while (InPathFollow._WaypointIndex < Waypoints.Num() - 1)
        {
            const auto& Waypoint = Waypoints[InPathFollow._WaypointIndex];

            const auto WithinArrivalRadius =
                FVector::Dist(CurrentLoc, Waypoint) <= WaypointArrivalRadius;

            const auto CrossedWaypointPlane = [&]() -> bool
            {
                const auto SegmentDir = (Waypoint - InPathFollow.Get_CurrentSegmentStart()).GetSafeNormal();

                // Degenerate segment: fall back to proximity rather than trust a zero normal.
                if (SegmentDir.IsNearlyZero())
                { return false; }

                return FVector::DotProduct(Waypoint - CurrentLoc, SegmentDir) < 0.0;
            }();

            if (NOT (WithinArrivalRadius || CrossedWaypointPlane))
            { break; }

            InPathFollow._CurrentSegmentStart = Waypoint;
            ++InPathFollow._WaypointIndex;
        }

        if (InPathFollow.Get_ProtectedLeadingWaypointCount() > 0 &&
            InPathFollow.Get_WaypointIndex() >=
                InPathFollow.Get_ProtectedLeadingWaypointCount())
        {
            InPathFollow._ProtectedLeadingWaypointCount = 0;
        }

        // Defensive: a single frame's offset can cross the whole path tail. The arrival side
        // effects stay owned by the final-stop branch below.
        if (InPathFollow._WaypointIndex >= Waypoints.Num())
        {
            DoZeroDesiredVelocity();
            return;
        }

        const auto& TargetLoc = Waypoints[InPathFollow._WaypointIndex];
        const auto ToTarget = TargetLoc - CurrentLoc;
        const auto DistanceToNext = ToTarget.Size();
        if (DistanceToNext <= KINDA_SMALL_NUMBER)
        {
            DoZeroDesiredVelocity();
            return;
        }
        const auto Direction = ToTarget / DistanceToNext;

        // Along the POLYLINE, not bird's-eye: a straight-line distance under-estimates whenever
        // the path bends, and the braking ramp below then overshoots.
        auto DistanceToFinal = DistanceToNext;
        for (auto i = InPathFollow._WaypointIndex; i < Waypoints.Num() - 1; ++i)
        {
            DistanceToFinal += FVector::Dist(Waypoints[i], Waypoints[i + 1]);
        }

        // _ActiveArrivalRadius was cached by HandleRequests from the params default or the
        // per-MoveTo override.
        const auto IsTargetingFinal = (InPathFollow._WaypointIndex == Waypoints.Num() - 1);
        if (IsTargetingFinal && DistanceToNext <= InPathFollow.Get_ActiveArrivalRadius())
        {
            InPathFollow._WaypointIndex = Waypoints.Num();
            DoZeroDesiredVelocity();

            auto NonConstHandle = InHandle;
            NonConstHandle.Try_Remove<FTag_CrowdAgent_Walking>();
            NonConstHandle.AddOrGet<FTag_CrowdAgent_Idle>();

            // A partial path's final waypoint is the closest REACHABLE point, not the goal —
            // reaching it is a FAILURE to reach the goal (verdict computed at install).
            if (InPathFollow.Get_ActivePathEndsShortOfGoal())
            {
                ck::crowd::Verbose(
                    TEXT("CrowdAgent [{}] walked its partial path to the end but the goal {} is unreachable from there — reporting OnGoalFailed"),
                    NonConstHandle, InPathFollow.Get_ActiveGoal());

                UUtils_Signal_CrowdAgent_OnGoalFailed::Broadcast(
                    NonConstHandle,
                    MakePayload(NonConstHandle));
                return;
            }

            UUtils_Signal_CrowdAgent_OnGoalReached::Broadcast(
                NonConstHandle,
                MakePayload(NonConstHandle));
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxTurnRate = InParams.Get_MaxTurnRate();

        // Inverse of the v²/(2a) stopping distance: the fastest we can still stop in time.
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        // Without this, a turning radius (v / MaxTurnRate) larger than the remaining distance makes
        // the agent physically unable to curve onto its goal, so it ORBITS instead. Only bites in
        // the final ~60cm at defaults.
        auto TurnRadiusSpeedCap = MaxSpeed;
        if (MaxTurnRate > 0.0f)
        {
            TurnRadiusSpeedCap = MaxTurnRate * DistanceToFinal;
        }

        // Raw target velocity — AccelClamp downstream brings it into the per-frame budget.
        const auto NewSpeed = FMath::Min3(MaxSpeed, BrakingSpeedCap, TurnRadiusSpeedCap);

        // INVARIANT: separation may REDIRECT forward motion, never CANCEL it. An opposing
        // component is redirected to pure lateral instead of being subtracted, because a desired
        // velocity that collapses to zero under pressure is a fixed point the AvoidanceSample
        // scoring below then locks in ("stay stopped" becomes the cheapest candidate).
        // See "Invariants this port must preserve" in CkCrowd/CLAUDE.md.
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
                // else: Direction is near-vertical. Leave the raw force rather than invent an axis.
            }
        }

        const auto Combined = Direction * NewSpeed + SeparationVec;
        InDesired._Velocity = Combined.GetClampedToMaxSize(MaxSpeed);
    }
}

// --------------------------------------------------------------------------------------------------------------------
