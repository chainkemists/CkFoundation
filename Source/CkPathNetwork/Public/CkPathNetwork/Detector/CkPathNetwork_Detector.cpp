#include "CkPathNetwork_Detector.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::pathnetwork::
    Try_VectorizeDetectorMaskToRibbons(
        const UCk_PathNetwork_Detector_UE& InDetector,
        const FBox& InWorldBounds,
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams)
    -> FCk_PathNetwork_VectorizeResult
{
    auto HasOccupiedCell = false;
    if (InMask.Get_IsValidMask())
    {
        for (const auto Cell : InMask.Get_Occupancy())
        {
            if (Cell != 0)
            {
                HasOccupiedCell = true;
                break;
            }
        }
    }

    if (NOT HasOccupiedCell)
    {
        return Try_VectorizeMaskToRibbons(
            InMask,
            InParams,
            nullptr);
    }

    auto EvaluatorCreation =
        InDetector.Create_VectorizationSegmentEvaluator(
            InWorldBounds,
            InMask);
    if (NOT EvaluatorCreation._Succeeded)
    {
        auto Result = FCk_PathNetwork_VectorizeResult{};
        Result._Succeeded = false;
        Result._FailureReason = EvaluatorCreation._FailureReason.IsEmpty()
            ? TEXT("Detector could not create its vectorization segment evaluator")
            : MoveTemp(EvaluatorCreation._FailureReason);
        return Result;
    }

    return Try_VectorizeMaskToRibbons(
        InMask,
        InParams,
        EvaluatorCreation._Evaluator.Get());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::pathnetwork::
    Get_AreAllRibbonSourcesGenerated(
        const TArray<FCk_PathNetwork_Ribbon>& InRibbons)
    -> bool
{
    for (const auto& Ribbon : InRibbons)
    {
        if (Ribbon.Get_Source() != ECk_PathNetwork_RibbonSource::Generated)
        { return false; }
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Create_VectorizationSegmentEvaluator(
        const FBox& InWorldBounds,
        const FCk_PathNetwork_DetectionMask& InMask) const
    -> FCk_PathNetwork_VectorizationEvaluatorCreationResult
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Validate_DetectionBounds_Implementation(
        const FBox& InWorldBounds) const
    -> FCk_PathNetwork_DetectorBoundsValidationResult
{
    auto Result = FCk_PathNetwork_DetectorBoundsValidationResult{};
    const auto BoundsAreValid =
        InWorldBounds.IsValid != 0
        && NOT InWorldBounds.Min.ContainsNaN()
        && NOT InWorldBounds.Max.ContainsNaN()
        && InWorldBounds.Max.X > InWorldBounds.Min.X
        && InWorldBounds.Max.Y > InWorldBounds.Min.Y
        && InWorldBounds.Max.Z > InWorldBounds.Min.Z;
    if (NOT BoundsAreValid)
    {
        Result.Set_Succeeded(false);
        Result.Set_FailureReason(
            TEXT("Detection bounds must be finite and ordered on every axis"));
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Process_GeneratedRibbons_WithVectorizeParams(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons,
        const FCk_PathNetwork_VectorizeParams&) const
    -> FCk_PathNetwork_DetectorProcessResult
{
    return Process_GeneratedRibbons(
        InWorldBounds,
        InGeneratedWorldRibbons);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Get_DetectionMask_Implementation(
        const FBox& InWorldBounds) const
    -> FCk_PathNetwork_DetectionMask
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Process_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
    -> FCk_PathNetwork_DetectorProcessResult
{
    auto Result = FCk_PathNetwork_DetectorProcessResult{};
    Result.Set_GeneratedWorldRibbons(InGeneratedWorldRibbons);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_Detector_UE::
    Validate_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
    -> FCk_PathNetwork_DetectorValidationResult
{
    auto Result = FCk_PathNetwork_DetectorValidationResult{};
    if (NOT ck::pathnetwork::Get_AreAllRibbonSourcesGenerated(
        InGeneratedWorldRibbons))
    {
        Result.Set_Succeeded(false);
        Result.Set_FailureReason(
            TEXT("Detector generated-output channel contains a non-Generated ribbon"));
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
