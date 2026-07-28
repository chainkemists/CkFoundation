#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
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

    CKPATHNETWORK_API auto
    Is_PointInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InPoint,
        float InTolerance = 1.0f)
        -> bool;

    CKPATHNETWORK_API auto
    Is_SegmentInsideRibbonRun(
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        const FVector& InFrom,
        const FVector& InTo,
        float InSampleSpacing = 10.0f,
        float InTolerance = 1.0f)
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
