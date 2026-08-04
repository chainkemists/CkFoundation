#include "CkVoxelNav_Path_Refine.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Raycast.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_path_refine
{
    // Centripetal parameterisation. The uniform form (alpha 0) cusps and self-intersects exactly where a
    // pruned voxel path is sharpest - at the corner it could not prune away - so it is the wrong default here.
    constexpr auto CentripetalAlpha = 0.5;

    constexpr auto SmallestUsefulKnotInterval = UE_DOUBLE_KINDA_SMALL_NUMBER;

    auto
        Get_KnotInterval(
            const FVector& InFrom,
            const FVector& InTo)
        -> double
    {
        const auto Delta = InTo - InFrom;

        return FMath::Pow(Delta | Delta, CentripetalAlpha * 0.5);
    }

    auto
        Get_CatmullRomPoint(
            const FVector& InP0,
            const FVector& InP1,
            const FVector& InP2,
            const FVector& InP3,
            double InAlpha)
        -> FVector
    {
        constexpr auto T0 = 0.0;

        const auto T1 = T0 + Get_KnotInterval(InP0, InP1);
        const auto T2 = T1 + Get_KnotInterval(InP1, InP2);
        const auto T3 = T2 + Get_KnotInterval(InP2, InP3);

        const auto KnotsAreDistinct =
            (T1 - T0) > SmallestUsefulKnotInterval &&
            (T2 - T1) > SmallestUsefulKnotInterval &&
            (T3 - T2) > SmallestUsefulKnotInterval;

        // Two coincident control points collapse a knot interval, and every blend below divides by one.
        if (NOT KnotsAreDistinct)
        { return FMath::Lerp(InP1, InP2, InAlpha); }

        const auto T = FMath::Lerp(T1, T2, InAlpha);

        const auto A1 = (T1 - T) / (T1 - T0) * InP0 + (T - T0) / (T1 - T0) * InP1;
        const auto A2 = (T2 - T) / (T2 - T1) * InP1 + (T - T1) / (T2 - T1) * InP2;
        const auto A3 = (T3 - T) / (T3 - T2) * InP2 + (T - T2) / (T3 - T2) * InP3;

        const auto B1 = (T2 - T) / (T2 - T0) * A1 + (T - T0) / (T2 - T0) * A2;
        const auto B2 = (T3 - T) / (T3 - T1) * A2 + (T - T1) / (T3 - T1) * A3;

        return (T2 - T) / (T2 - T1) * B1 + (T - T1) / (T2 - T1) * B2;
    }

    // The curve needs a control point beyond each end of the path. Reflecting the second point through the
    // first is what makes the tangent there continue the path instead of veering off toward the origin.
    auto
        Make_PaddedControlPoints(
            const TArray<FVector>& InWaypoints)
        -> TArray<FVector>
    {
        constexpr auto VirtualEndpointCount = 2;

        auto Padded = TArray<FVector>{};
        Padded.Reserve(InWaypoints.Num() + VirtualEndpointCount);

        Padded.Emplace(2.0 * InWaypoints[0] - InWaypoints[1]);
        Padded.Append(InWaypoints);
        Padded.Emplace(2.0 * InWaypoints.Last() - InWaypoints.Last(1));

        return Padded;
    }

    auto
        Get_IsSubdividedSpanTraversable(
            const ck::voxelnav::FOctree& InOctree,
            const FVector& InSpanStart,
            const TArray<FVector>& InInteriorPoints,
            const FVector& InSpanEnd)
        -> bool
    {
        auto PreviousPoint = InSpanStart;

        for (const auto& InteriorPoint : InInteriorPoints)
        {
            if (NOT ck::voxelnav::Get_IsPositionFree(InOctree, InteriorPoint))
            { return false; }

            if (ck::voxelnav::Get_IsSegmentBlocked(InOctree, PreviousPoint, InteriorPoint))
            { return false; }

            PreviousPoint = InteriorPoint;
        }

        return NOT ck::voxelnav::Get_IsSegmentBlocked(InOctree, PreviousPoint, InSpanEnd);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Get_PathLength(
            const TArray<FVector>& InWaypoints)
        -> float
    {
        auto Length = 0.0;

        for (auto WaypointIdx = 1; WaypointIdx < InWaypoints.Num(); ++WaypointIdx)
        { Length += FVector::Dist(InWaypoints[WaypointIdx - 1], InWaypoints[WaypointIdx]); }

        return static_cast<float>(Length);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Prune_VisibleWaypoints(
            const FOctree& InOctree,
            const TArray<FVector>& InWaypoints)
        -> TArray<FVector>
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Prune_VisibleWaypoints);

        constexpr auto ShortestPrunablePath = 3;

        if (InWaypoints.Num() < ShortestPrunablePath)
        { return InWaypoints; }

        const auto LastIdx = InWaypoints.Num() - 1;

        auto Pruned = TArray<FVector>{};
        Pruned.Reserve(InWaypoints.Num());
        Pruned.Emplace(InWaypoints[0]);

        auto AnchorIdx = 0;

        while (AnchorIdx < LastIdx)
        {
            // Keeping the immediate successor is the floor, not a failure: it is the step the search itself
            // produced, so a span the octree will not vouch for stays exactly as planned.
            auto NextIdx = AnchorIdx + 1;

            for (auto CandidateIdx = LastIdx; CandidateIdx > AnchorIdx + 1; --CandidateIdx)
            {
                if (Get_IsSegmentBlocked(InOctree, InWaypoints[AnchorIdx], InWaypoints[CandidateIdx]))
                { continue; }

                NextIdx = CandidateIdx;
                break;
            }

            Pruned.Emplace(InWaypoints[NextIdx]);
            AnchorIdx = NextIdx;
        }

        return Pruned;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Smooth_Waypoints(
            const FOctree& InOctree,
            const TArray<FVector>& InWaypoints,
            int32 InSubdivisions)
        -> TArray<FVector>
    {
        using namespace ck_voxelnav_path_refine;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Smooth_Waypoints);

        constexpr auto ShortestSmoothablePath = 2;
        constexpr auto FewestSubdivisionsThatAddAPoint = 2;

        if (InWaypoints.Num() < ShortestSmoothablePath || InSubdivisions < FewestSubdivisionsThatAddAPoint)
        { return InWaypoints; }

        const auto Padded = Make_PaddedControlPoints(InWaypoints);
        const auto InteriorPointsPerSpan = InSubdivisions - 1;

        auto Smoothed = TArray<FVector>{};
        Smoothed.Reserve(InWaypoints.Num() + (InWaypoints.Num() - 1) * InteriorPointsPerSpan);
        Smoothed.Emplace(InWaypoints[0]);

        auto InteriorPoints = TArray<FVector>{};
        InteriorPoints.Reserve(InteriorPointsPerSpan);

        for (auto SpanIdx = 0; SpanIdx < InWaypoints.Num() - 1; ++SpanIdx)
        {
            const auto& SpanStart = InWaypoints[SpanIdx];
            const auto& SpanEnd = InWaypoints[SpanIdx + 1];

            InteriorPoints.Reset();

            for (auto StepIdx = 1; StepIdx <= InteriorPointsPerSpan; ++StepIdx)
            {
                InteriorPoints.Emplace(Get_CatmullRomPoint(
                    Padded[SpanIdx], SpanStart, SpanEnd, Padded[SpanIdx + 3],
                    static_cast<double>(StepIdx) / static_cast<double>(InSubdivisions)));
            }

            if (Get_IsSubdividedSpanTraversable(InOctree, SpanStart, InteriorPoints, SpanEnd))
            { Smoothed.Append(InteriorPoints); }

            Smoothed.Emplace(SpanEnd);
        }

        return Smoothed;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Refine_Waypoints(
            const FOctree& InOctree,
            const TArray<FVector>& InWaypoints,
            const FPathRefineParams& InParams)
        -> FPathRefineResult
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Refine_Waypoints);

        auto Result = FPathRefineResult{};

        Result._Waypoints = InWaypoints;
        Result._RawWaypointCount = InWaypoints.Num();
        Result._RawLengthUu = Get_PathLength(InWaypoints);
        Result._RefinedLengthUu = Result._RawLengthUu;

        // Every step here is an octree query, so an octree with nothing in it can refine nothing. The path is
        // handed back whole rather than emptied: it is still the search's own answer.
        if (NOT InOctree.Get_IsValid())
        { return Result; }

        if (InParams._VisibilityPruning == ECk_EnableDisable::Enable)
        { Result._Waypoints = Prune_VisibleWaypoints(InOctree, Result._Waypoints); }

        if (InParams._Smoothing == ECk_EnableDisable::Enable)
        { Result._Waypoints = Smooth_Waypoints(InOctree, Result._Waypoints, InParams._SmoothingSubdivisions); }

        Result._RefinedLengthUu = Get_PathLength(Result._Waypoints);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
