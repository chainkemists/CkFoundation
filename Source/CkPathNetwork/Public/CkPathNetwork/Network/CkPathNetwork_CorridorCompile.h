#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    inline constexpr auto RibbonContainmentSampleSpacingCm = 10.0f;
    inline constexpr auto RibbonContainmentToleranceCm = 2.0f;

    // Pure, world-free controls for compiling one contiguous on-ribbon route run. The compiler
    // preserves the selected source geometry, so large waypoint spacing cannot delete a corner.
    struct CKPATHNETWORK_API FCorridorCompileParams
    {
        float _SideKeepingFraction = 0.5f;
        float _WaypointSpacing = 250.0f;
        float _CornerSmoothingDistance = 150.0f;
        bool _RampSideOffsetAtStart = true;
        bool _RampSideOffsetAtEnd = true;
    };

    // Filled only when segment containment fails. The nearest ribbon sample separates a planar
    // escape from a vertical navmesh projection mismatch without changing corridor behavior.
    struct CKPATHNETWORK_API FRibbonContainmentFailure
    {
        FVector _Sample = FVector::ZeroVector;
        FVector _ClosestRibbonPoint = FVector::ZeroVector;
        int32 _SampleIndex = INDEX_NONE;
        int32 _SampleCount = 0;
        int32 _SpanIndex = INDEX_NONE;
        int32 _EdgeId = INDEX_NONE;
        float _Distance3D = 0.0f;
        float _Distance2D = 0.0f;
        float _VerticalDistance = 0.0f;
        float _AllowedDistance = 0.0f;
    };

    CKPATHNETWORK_API auto
    Is_PointInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InPoint,
        float InTolerance = RibbonContainmentToleranceCm)
        -> bool;

    CKPATHNETWORK_API auto
    Is_SegmentInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InFrom,
        const FVector& InTo,
        float InSampleSpacing = RibbonContainmentSampleSpacingCm,
        float InTolerance = RibbonContainmentToleranceCm,
        FRibbonContainmentFailure* OutFailure = nullptr)
        -> bool;

    // Validates a complete waypoint path against one ribbon run while building the route-local
    // containment acceleration structure only once. OutFailureSegmentIndex identifies the first
    // rejected segment when supplied.
    CKPATHNETWORK_API auto
    Is_PathInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        TConstArrayView<FVector> InWaypoints,
        float InSampleSpacing = RibbonContainmentSampleSpacingCm,
        float InTolerance = RibbonContainmentToleranceCm,
        FRibbonContainmentFailure* OutFailure = nullptr,
        int32* OutFailureSegmentIndex = nullptr)
        -> bool;

    // Compiles all adjacent on-ribbon spans together. Eligible corners receive bounded quadratic
    // fillets; every emitted segment is checked against the union of the selected ribbon spans.
    // If smoothing or side keeping does not fit, the compiler degrades to the corner-preserving
    // centerline instead of cutting across the corridor.
    CKPATHNETWORK_API auto
    Compile_OnRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FCorridorCompileParams& InParams)
        -> TArray<FVector>;
}

// --------------------------------------------------------------------------------------------------------------------
