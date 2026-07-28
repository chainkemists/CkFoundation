#include "CkPathNetwork_Detector.h"

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
