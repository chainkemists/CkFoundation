#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPathNetwork/Network/CkPathNetwork_Vectorize.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_Detector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Result of the detector's pre-rasterization bounds admission pass. Detectors advertise their
// operational limits here so editor tooling and runtime callers can reject an unaffordable request
// before invoking Get_DetectionMask.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_DetectorBoundsValidationResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_DetectorBoundsValidationResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = true;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

public:
    CK_PROPERTY(_Succeeded);
    CK_PROPERTY(_FailureReason);
};

// --------------------------------------------------------------------------------------------------------------------
// Result of the detector's optional post-vectorization validation pass. Detectors that need a
// stronger contract than raster-cell admission can reject generated geometry before it is
// published (for example, by sampling every emitted sidewalk segment against game-owned source
// geometry). A rejection is atomic: editor bake and runtime rebuild keep the previous network.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_DetectorValidationResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_DetectorValidationResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = true;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _RibbonIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _SegmentIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _SampleIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _SampleLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _SamplesChecked = 0;

public:
    CK_PROPERTY(_Succeeded);
    CK_PROPERTY(_FailureReason);
    CK_PROPERTY(_RibbonIndex);
    CK_PROPERTY(_SegmentIndex);
    CK_PROPERTY(_SampleIndex);
    CK_PROPERTY(_SampleLocation);
    CK_PROPERTY(_SamplesChecked);
};

// --------------------------------------------------------------------------------------------------------------------

// Result of the detector's optional game-specific generated-ribbon processing pass. The default
// returns vectorizer output unchanged. A detector may split or filter Generated ribbons to enforce
// source-specific continuity before the separate validation hook admits them.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_DetectorProcessResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_DetectorProcessResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = true;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetwork_Ribbon> _GeneratedWorldRibbons;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _UnsupportedSegmentCount = 0;

public:
    CK_PROPERTY(_Succeeded);
    CK_PROPERTY(_FailureReason);
    CK_PROPERTY(_GeneratedWorldRibbons);
    CK_PROPERTY(_UnsupportedSegmentCount);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    // Admission fence for detector processing output. Authored ribbons are owned by the actor and
    // may never re-enter through the replaceable generated channel.
    CKPATHNETWORK_API auto
    Get_AreAllRibbonSourcesGenerated(
        const TArray<FCk_PathNetwork_Ribbon>& InRibbons) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
// Per-game sidewalk detection. The framework never knows HOW paths are marked in a level — a game
// subclasses this (C++, Blueprint, or AngelScript) and rasterizes its own source of truth (e.g.
// landscape paint-layer weights) into an occupancy mask. Everything downstream (vectorize -> build
// -> route -> follow) is game-agnostic.
//
// The detector must be runtime-callable if the game wants runtime rebuilds; editor-only detectors
// are fine for games that only bake in-editor.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CKPATHNETWORK_API UCk_PathNetwork_Detector_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PathNetwork_Detector_UE);

public:
    // Optional C++-only exact segment proof policy. The returned evaluator is owned by one
    // synchronous vectorization pass; null means the generic occupancy/height policy is sufficient.
    // Reflected detector subclasses retain legacy behavior unless their native base overrides this.
    virtual auto
    Create_VectorizationSegmentEvaluator(
        const FBox& InWorldBounds,
        const FCk_PathNetwork_DetectionMask& InMask) const
        -> FCk_PathNetwork_VectorizationEvaluatorCreationResult;

    // Admit or reject bounds before rasterization. Override when the detector has a resolution,
    // memory, source-coverage, or other bounds-dependent operating limit.
    UFUNCTION(BlueprintCallable,
              BlueprintNativeEvent,
              Category = "Ck|PathNetwork|Detector",
              DisplayName = "[Ck][PathNetwork] Validate Detection Bounds")
    FCk_PathNetwork_DetectorBoundsValidationResult
    Validate_DetectionBounds(
        const FBox& InWorldBounds) const;

    // Rasterize path/sidewalk coverage inside InWorldBounds. Return an invalid mask (SizeX/Y == 0)
    // to report "nothing detected".
    UFUNCTION(BlueprintCallable,
              BlueprintNativeEvent,
              Category = "Ck|PathNetwork|Detector",
              DisplayName = "[Ck][PathNetwork] Get Detection Mask")
    FCk_PathNetwork_DetectionMask
    Get_DetectionMask(
        const FBox& InWorldBounds) const;

    // Optional game-owned transform between generic vectorization and validation/publication.
    // The default returns InGeneratedWorldRibbons unchanged.
    UFUNCTION(BlueprintCallable,
              BlueprintNativeEvent,
              Category = "Ck|PathNetwork|Detector",
              DisplayName = "[Ck][PathNetwork] Process Generated Ribbons")
    FCk_PathNetwork_DetectorProcessResult
    Process_GeneratedRibbons(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const;

    // Optional generated-geometry admission hook. The default accepts all vectorizer output.
    // Override when cell-center admission alone cannot prove that the full emitted segments are
    // usable. Authored ribbons are never passed to this hook.
    UFUNCTION(BlueprintCallable,
              BlueprintNativeEvent,
              Category = "Ck|PathNetwork|Detector",
              DisplayName = "[Ck][PathNetwork] Validate Generated Ribbons")
    FCk_PathNetwork_DetectorValidationResult
    Validate_GeneratedRibbons(
        const FBox& InWorldBounds,
        const TArray<FCk_PathNetwork_Ribbon>& InGeneratedWorldRibbons) const;
};

// --------------------------------------------------------------------------------------------------------------------
