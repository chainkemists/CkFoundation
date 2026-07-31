#pragma once

#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"

#include "CkPathNetwork_AuthoringTestDetector.generated.h"

class AActor;

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_PathNetwork_AuthoringTestDetectorBehavior : uint8
{
    Succeeds,
    BoundsValidationFails,
    ProcessFails,
    ValidationFails
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Transient)
class UCk_PathNetwork_AuthoringTestDetector : public UCk_PathNetwork_Detector_UE
{
    GENERATED_BODY()

public:
    auto
    Validate_DetectionBounds_Implementation(
        const FBox& InWorldBounds) const
        -> FCk_PathNetwork_DetectorBoundsValidationResult override;

    auto
    Get_DetectionMask_Implementation(
        const FBox& InWorldBounds) const -> FCk_PathNetwork_DetectionMask override;

    auto
    Process_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
        -> FCk_PathNetwork_DetectorProcessResult override;

    auto
    Validate_GeneratedRibbons_Implementation(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const
        -> FCk_PathNetwork_DetectorValidationResult override;

    auto
    Set_Behavior(
        ECk_PathNetwork_AuthoringTestDetectorBehavior InBehavior) -> void;

    auto
    Set_ConfigurationRevision(
        int32 InConfigurationRevision) -> void;

    auto
    Set_LocationSource(
        AActor* InLocationSource) -> void;

    auto
    Get_CallbackCount() const -> int32;

private:
    UPROPERTY()
    ECk_PathNetwork_AuthoringTestDetectorBehavior _Behavior =
        ECk_PathNetwork_AuthoringTestDetectorBehavior::Succeeds;

    UPROPERTY()
    int32 _ConfigurationRevision = 0;

    UPROPERTY()
    TObjectPtr<AActor> _LocationSource;

    mutable int32 _CallbackCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------
