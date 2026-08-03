#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>
#include <Templates/UniquePtr.h>

class UCk_PathNetwork_Detector_UE;

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_PathNetwork_VectorizationSegmentDecision : uint8
{
    Supported,
    Unsupported,
    EvaluationFailed
};

struct CKPATHNETWORK_API FCk_PathNetwork_VectorizationSegmentResult
{
    ECk_PathNetwork_VectorizationSegmentDecision _Decision =
        ECk_PathNetwork_VectorizationSegmentDecision::Supported;
    FString _FailureReason;
};

// Per-generation, game-owned proof policy for emitted centerline segments. Instances are created
// immediately after mask detection and live only for the synchronous vectorization call, so they
// may safely own transient world-query acceleration data without storing it on the detector UObject.
class CKPATHNETWORK_API ICk_PathNetwork_VectorizationSegmentEvaluator
{
public:
    virtual ~ICk_PathNetwork_VectorizationSegmentEvaluator() = default;

    virtual auto
    Evaluate_Segment(
        const FVector& InStart,
        const FVector& InEnd)
        -> FCk_PathNetwork_VectorizationSegmentResult = 0;
};

struct CKPATHNETWORK_API FCk_PathNetwork_VectorizationEvaluatorCreationResult
{
    bool _Succeeded = true;
    FString _FailureReason;
    TUniquePtr<ICk_PathNetwork_VectorizationSegmentEvaluator> _Evaluator;
};

struct CKPATHNETWORK_API FCk_PathNetwork_VectorizeResult
{
    bool _Succeeded = true;
    FString _FailureReason;
    TArray<FCk_PathNetwork_Ribbon> _Ribbons;
};

// --------------------------------------------------------------------------------------------------------------------
// Mask -> ribbons. Pure math, runtime-callable, no ECS/world dependency. Pipeline: chamfer distance
// transform (half-width estimation) -> Zhang-Suen thinning -> chain tracing between junction/end
// pixels -> Douglas-Peucker. Output ribbons are Generated-sourced and get fresh ids.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    // Simplify an already-generated ribbon while bounding the visible corridor-footprint error.
    // Zero preserves every source point. Positive values preserve exact endpoints and bound the
    // summed planar-centerline plus half-width error, with height bounded separately.
    CKPATHNETWORK_API auto
    Simplify_RibbonPoints(
        const TArray<FCk_PathNetwork_RibbonPoint>& InPoints,
        float InTolerance,
        TFunctionRef<bool(int32, int32)> InIsChordSupported)
        -> TArray<FCk_PathNetwork_RibbonPoint>;

    // Convenience overload for callers whose source admits every tolerance-compliant chord.
    CKPATHNETWORK_API auto
    Simplify_RibbonPoints(
        const TArray<FCk_PathNetwork_RibbonPoint>& InPoints,
        float InTolerance) -> TArray<FCk_PathNetwork_RibbonPoint>;

    CKPATHNETWORK_API auto
    Try_VectorizeMaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams,
        ICk_PathNetwork_VectorizationSegmentEvaluator* InSegmentEvaluator)
        -> FCk_PathNetwork_VectorizeResult;

    CKPATHNETWORK_API auto
    Vectorize_MaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams) -> TArray<FCk_PathNetwork_Ribbon>;

    // Shared editor/runtime bridge. The detector may create a synchronous C++ segment evaluator;
    // Blueprint/AngelScript-only detectors inherit the default null evaluator and preserve legacy
    // mask-only vectorization.
    CKPATHNETWORK_API auto
    Try_VectorizeDetectorMaskToRibbons(
        const UCk_PathNetwork_Detector_UE& InDetector,
        const FBox& InWorldBounds,
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams)
        -> FCk_PathNetwork_VectorizeResult;
}

// --------------------------------------------------------------------------------------------------------------------
