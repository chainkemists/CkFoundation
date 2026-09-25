#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include "Templates/Function.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_path_follow_algorithm
{
    // Answers "can the agent walk the straight chord from A to B". Supplied by the caller because
    // this header owns no world access; a caller with no nav data supplies the always-true form.
    using FIsChordNavigable = TFunctionRef<bool(const FVector& InFrom, const FVector& InTo)>;

    // A path can finish planning after the agent has moved beyond one or more leading corners.
    // Select the first still-forward waypoint using the outgoing segment direction, matching
    // UPathFollowingComponent's passed-plane test. The final waypoint is never skipped.
    inline auto SkipAlreadyPassedLeadingWaypoints(
        const FVector InAgentLocation,
        const TConstArrayView<FVector> InWaypoints,
        int32& InOutWaypointIndex,
        FVector& InOutCurrentSegmentStart,
        const int32 InProtectedLeadingWaypointCount,
        FIsChordNavigable InIsChordNavigable) -> int32
    {
        const auto CursorIsValid =
            InOutWaypointIndex >= 0 &&
            InOutWaypointIndex < InWaypoints.Num();

        auto WaypointsAreValid = CursorIsValid;
        if (CursorIsValid)
        {
            for (auto Index = InOutWaypointIndex; Index < InWaypoints.Num(); ++Index)
            {
                if (InWaypoints[Index].ContainsNaN())
                {
                    WaypointsAreValid = false;
                    break;
                }
            }
        }

        const auto InputsAreValid =
            NOT InAgentLocation.ContainsNaN() &&
            CursorIsValid &&
            WaypointsAreValid &&
            InProtectedLeadingWaypointCount >= 0 &&
            InProtectedLeadingWaypointCount <= InWaypoints.Num();
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd path-install inputs "
                  "(agent_location [{}], waypoint_count [{}], waypoint_index [{}], protected_leading [{}])"),
            InAgentLocation,
            InWaypoints.Num(),
            InOutWaypointIndex,
            InProtectedLeadingWaypointCount)
        { return 0; }

        const auto StartingWaypointIndex = InOutWaypointIndex;
        while (InOutWaypointIndex < InWaypoints.Num() - 1)
        {
            if (InOutWaypointIndex < InProtectedLeadingWaypointCount)
            { break; }

            const auto& Corner = InWaypoints[InOutWaypointIndex];
            const auto& NextWaypoint = InWaypoints[InOutWaypointIndex + 1];
            const auto OnwardDirection = (NextWaypoint - Corner).GetSafeNormal();
            if (OnwardDirection.IsNearlyZero())
            { break; }

            if (FVector::DotProduct(Corner - InAgentLocation, OnwardDirection) >= 0.0)
            { break; }

            // The passed-plane test is unbounded perpendicular to the segment, so an agent standing
            // BESIDE a corner reads as past it while the chord from where it actually stands to the
            // following waypoint cuts through geometry the planner routed around. Installing that
            // aim points steering into a wall the navmesh constraint then eats whole. The corner is
            // on-mesh by construction, so keeping it costs nothing but a turn.
            if (NOT InIsChordNavigable(InAgentLocation, NextWaypoint))
            { break; }

            InOutCurrentSegmentStart = Corner;
            ++InOutWaypointIndex;
        }

        return InOutWaypointIndex - StartingWaypointIndex;
    }

    // For callers with no nav data to test a chord against — the navmesh constraint is a
    // pass-through in such a world too, so unconditional skipping is the matching behaviour.
    inline auto SkipAlreadyPassedLeadingWaypoints(
        const FVector InAgentLocation,
        const TConstArrayView<FVector> InWaypoints,
        int32& InOutWaypointIndex,
        FVector& InOutCurrentSegmentStart,
        const int32 InProtectedLeadingWaypointCount = 0) -> int32
    {
        auto EveryChordIsNavigable = [](const FVector&, const FVector&) -> bool
        { return true; };

        return SkipAlreadyPassedLeadingWaypoints(
            InAgentLocation,
            InWaypoints,
            InOutWaypointIndex,
            InOutCurrentSegmentStart,
            InProtectedLeadingWaypointCount,
            EveryChordIsNavigable);
    }

    // Status alone cannot decide this: Recast re-projects the goal through the query filter
    // (FPImplRecastNavMesh::InitPathfinding), so a goal just inside an excluded area comes back Ready,
    // ending at its edge. A Ready route is judged only against a goal projected onto the surface,
    // because a raw goal's Z need not lie on it.
    inline auto Get_RouteEndsShortOfGoal(
        const ECk_Nav_PathStatus InStatus,
        const TConstArrayView<FVector> InWaypoints,
        const FVector& InGoal,
        const bool InGoalIsOnSurface,
        const float InArrivalRadius) -> bool
    {
        if (InWaypoints.IsEmpty())
        { return false; }

        const auto IsJudged =
            InStatus == ECk_Nav_PathStatus::Partial ||
            (InStatus == ECk_Nav_PathStatus::Ready && InGoalIsOnSurface);
        if (NOT IsJudged)
        { return false; }

        return FVector::Dist(InWaypoints.Last(), InGoal) > InArrivalRadius;
    }
}

// --------------------------------------------------------------------------------------------------------------------
