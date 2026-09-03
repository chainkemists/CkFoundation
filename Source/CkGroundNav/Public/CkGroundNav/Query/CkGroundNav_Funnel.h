#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// THE ONE FUNNEL. The flood fill measures with it and the path search string-pulls with it; a
// second implementation anywhere is a defect. Pure geometry: no field, no world, no registry, safe
// on any thread, allocates only into the caller's arrays.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * One portal interval as the body walks through it: left and right are relative to the
     * direction of travel, and both ends carry the surface height at that point so a waypoint
     * placed on the interval has a height to stand at.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_FunnelPortal
    {
    public:
        FVector _Left = FVector::ZeroVector;
        FVector _Right = FVector::ZeroVector;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The shortest path from a start to an end through an ordered sequence of portal intervals — the
     * simple stupid funnel, in XY, with heights interpolated on whichever interval a waypoint lands on.
     *
     * Radius inset: every interval is shrunk by InRadiusUu from BOTH ends toward its midpoint before
     * funnelling, and an interval narrower than twice the radius collapses to its midpoint — a path that
     * was admitted through it already had its clearance checked, so the collapse is a formality and
     * never a wrong answer. A zero-length interval is an ordinary portal the path passes through at
     * that point. A start and end with no portals between them is a straight line.
     *
     * Waypoints: the start, every corner the string bends at, the end — never a duplicate, never NaN.
     * Returns the XY length of the string.
     */
    CKGROUNDNAV_API auto
    Get_StringPull(
        const FVector&                              InStart,
        const FVector&                              InEnd,
        TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
        float                                       InRadiusUu,
        TArray<FVector>&                            OutWaypoints) -> double;

    /**
     * The shortest path from a start THROUGH the portals to the nearest point of one more interval —
     * the flood fill's relaxation step, where the goal is a crossing and not yet a point on it.
     *
     * The target is treated as the last portal. Once every portal is consumed the apex sees a wedge of
     * the target; the answer is the point of the target inside that wedge nearest the apex, so the last
     * leg is straight and the length is exact for the portal sequence given. The same inset rule
     * applies to the target. OutPoint carries the height interpolated on the target.
     */
    CKGROUNDNAV_API auto
    Get_StringPull_ToSegment(
        const FVector&                              InStart,
        TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
        const FCk_GroundNav_FunnelPortal&           InTarget,
        float                                       InRadiusUu,
        FVector&                                    OutPoint,
        TArray<FVector>&                            OutWaypoints) -> double;
}

// --------------------------------------------------------------------------------------------------------------------
