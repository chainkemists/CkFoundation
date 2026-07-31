#include "Tests/CkPathNetwork_AuthoringTestDetector.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Validate_DetectionBounds_Implementation(
        const FBox& InWorldBounds) const
    -> FCk_PathNetwork_DetectorBoundsValidationResult
{
    auto Result = FCk_PathNetwork_DetectorBoundsValidationResult{};
    if (_Behavior == ECk_PathNetwork_AuthoringTestDetectorBehavior::BoundsValidationFails)
    {
        Result.Set_Succeeded(false);
        Result.Set_FailureReason(TEXT("Intentional bounds validation failure"));
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Get_DetectionMask_Implementation(
    const FBox& InWorldBounds) const
    -> FCk_PathNetwork_DetectionMask
{
    ++_CallbackCount;
    auto Mask = FCk_PathNetwork_DetectionMask{
        InWorldBounds.Min,
        100.0f,
        2,
        2};
    Mask.Set_Occupancy({1, 1, 1, 1});
    return Mask;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Process_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
    -> FCk_PathNetwork_DetectorProcessResult
{
    auto Result = FCk_PathNetwork_DetectorProcessResult{};
    if (_Behavior == ECk_PathNetwork_AuthoringTestDetectorBehavior::ProcessFails)
    {
        Result.Set_Succeeded(false);
        Result.Set_FailureReason(TEXT("Intentional process failure"));
        return Result;
    }

    const auto SourceOffset = IsValid(_LocationSource)
        ? _LocationSource->GetActorLocation()
        : FVector::ZeroVector;
    auto Ribbon = FCk_PathNetwork_Ribbon{
        TArray<FCk_PathNetwork_RibbonPoint>{
            FCk_PathNetwork_RibbonPoint{
                InWorldBounds.Min + FVector{25.0f, 25.0f, 0.0f} + SourceOffset, 50.0f},
            FCk_PathNetwork_RibbonPoint{
                InWorldBounds.Max - FVector{25.0f, 25.0f, 0.0f} + SourceOffset, 50.0f}}};
    Ribbon.Set_Source(ECk_PathNetwork_RibbonSource::Generated);
    Result.Set_GeneratedWorldRibbons({Ribbon});
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Validate_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
    -> FCk_PathNetwork_DetectorValidationResult
{
    auto Result = FCk_PathNetwork_DetectorValidationResult{};
    if (_Behavior == ECk_PathNetwork_AuthoringTestDetectorBehavior::ValidationFails)
    {
        Result.Set_Succeeded(false);
        Result.Set_FailureReason(TEXT("Intentional validation failure"));
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Set_Behavior(
        const ECk_PathNetwork_AuthoringTestDetectorBehavior InBehavior) -> void
{
    _Behavior = InBehavior;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Set_ConfigurationRevision(
        const int32 InConfigurationRevision) -> void
{
    _ConfigurationRevision = InConfigurationRevision;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Set_LocationSource(
        AActor* InLocationSource)
    -> void
{
    _LocationSource = InLocationSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetwork_AuthoringTestDetector::
    Get_CallbackCount() const
    -> int32
{
    return _CallbackCount;
}

// --------------------------------------------------------------------------------------------------------------------
