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
    // Re-discovers parameters, preserving existing overrides by name. The source asset is never modified.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|ParametricImage")
    void
    Set_SourceMaterial(
        UMaterialInterface* InSourceMaterial);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|ParametricImage")
    UMaterialInterface*
    Get_SourceMaterial() const;

    // Re-reads the parameter list from the source material — adds newly-found parameters, drops ones
    // that no longer exist. Override flags and values are preserved by name for overridden parameters.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Parameters")
    void
    Update_FromMaterial();

    // Clears every override, so the widget renders identically to the untouched source material.
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
    // InPreserveOverrides carries an existing overridden row's flag+value across, matched on name+type.
    auto DiscoverParameters(
        bool InPreserveOverrides) -> void;

    // No-op when the brush already holds a material — must not clobber the sequencer's "_Animated" MID.
    auto EnsureBrushHasMaterial() -> void;

    auto ApplyParametersToDynamicMaterial() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
