#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_path_follow_algorithm
{
    // A path can finish planning after the agent has moved beyond one or more leading corners.
    // Select the first still-forward waypoint using the outgoing segment direction, matching
    // UPathFollowingComponent's passed-plane test. The final waypoint is never skipped.
    inline auto SkipAlreadyPassedLeadingWaypoints(
        const FVector InAgentLocation,
        const TConstArrayView<FVector> InWaypoints,
        int32& InOutWaypointIndex,
        FVector& InOutCurrentSegmentStart) -> int32
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
            WaypointsAreValid;
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid crowd path-install inputs "
                 "(agent_location [{}], waypoint_count [{}], waypoint_index [{}])"),
            InAgentLocation,
            InWaypoints.Num(),
            InOutWaypointIndex)
        {}
        if (NOT InputsAreValid)
        { return 0; }

        const auto StartingWaypointIndex = InOutWaypointIndex;
        while (InOutWaypointIndex < InWaypoints.Num() - 1)
        {
            const auto& Corner = InWaypoints[InOutWaypointIndex];
            const auto OnwardDirection =
                (InWaypoints[InOutWaypointIndex + 1] - Corner).GetSafeNormal();
            if (OnwardDirection.IsNearlyZero())
            { break; }

            if (FVector::DotProduct(Corner - InAgentLocation, OnwardDirection) >= 0.0)
            { break; }

            InOutCurrentSegmentStart = Corner;
            ++InOutWaypointIndex;
        }

        return InOutWaypointIndex - StartingWaypointIndex;
    }
}

// --------------------------------------------------------------------------------------------------------------------
