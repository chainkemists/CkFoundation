#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkGraphics/CkGraphics_Common.h"

#include <CoreMinimal.h>
#include <Components/Image.h>

#include "CkParametricImage_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UMaterialInterface;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Image widget that exposes a Material / Material Instance's Scalar, Color and Texture parameters as
 * editable per-widget overrides (FCk_Material_Parameter). Auto-discovers parameters from the assigned
 * material, drives a per-widget Dynamic Material Instance, previews live in the UMG designer, and (because
 * it inherits UImage's FSlateBrush) is automatically animatable via the UMG Animator's material tracks.
 *
 * Non-destructive: the source Material/Instance asset on disk is never modified.
 */
UCLASS(meta = (DisplayName = "Ck Parametric Image"))
class CKUI_API UCk_ParametricImageWidget_UE : public UImage
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ParametricImageWidget_UE);

public:
    // Assigns the source material, seeds the inherited brush with it (so the UMG animator can discover
    // material tracks and a dynamic instance can be built), re-discovers parameters preserving any
    // existing overrides by name, and applies them. The source asset is never modified.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|ParametricImage")
    void
    Set_SourceMaterial(
        UMaterialInterface* InSourceMaterial);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|ParametricImage")
    UMaterialInterface*
    Get_SourceMaterial() const;

    // Re-reads the parameter list from the source material — adds newly-found parameters, drops ones
    // that no longer exist. Override flags and values are preserved by name for overridden parameters.
    // Category "Parameters" so the editor button groups with the _Parameters array in the details panel.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Parameters")
    void
    Update_FromMaterial();

    // Clears every override and re-captures each parameter's value from the material's defaults, so the
    // widget renders identically to the untouched source material.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Parameters")
    void
    Reset_ToMaterialDefaults();

public:
    auto SynchronizeProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material",
              meta = (AllowPrivateAccess = true, DisplayName = "Material"))
    TObjectPtr<UMaterialInterface> _SourceMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters",
              meta = (AllowPrivateAccess = true, TitleProperty = "_ParameterName"))
    TArray<FCk_Material_Parameter> _Parameters;

private:
    // Rebuilds the parameter array from _SourceMaterial. When InPreserveOverrides is true, an existing
    // overridden row's _Override/value is carried over for any parameter name+type that still exists.
    auto DiscoverParameters(
        bool InPreserveOverrides) -> void;

    // Seeds the inherited brush with _SourceMaterial only when the brush currently holds no material.
    // Leaves an existing material instance untouched (e.g. the sequencer's "_Animated" MID mid-playback).
    auto EnsureBrushHasMaterial() -> void;

    // Pushes every overridden parameter onto the per-widget Dynamic Material Instance.
    auto ApplyParametersToDynamicMaterial() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
