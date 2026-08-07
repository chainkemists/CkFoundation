#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkUsf/Stylize/CkUsf_ScreenDither_Params.h"

#include "CkUsf_ScreenDitherPreset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// An authored ScreenDither style (authorable in AngelScript via
// `asset DA_Foo of UCkUsf_ScreenDitherPreset {...}`, or as an editor data asset). Public fields mirror
// FCk_Usf_ScreenDither_Params one for one — the UCkUsf_OutlinePreset precedent — and Get_AsParams packs
// them into the value the subsystem actually owns.
UCLASS(BlueprintType)
class CKUSF_API UCkUsf_ScreenDitherPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_Usf_DitherPattern _Pattern = ECk_Usf_DitherPattern::Bayer4x4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 1, ClampMin = 1, UIMax = 32))
    float _PixelScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _DitherStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_EnableDisable _Animate = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.01, ClampMin = 0.001, EditCondition = "_Animate == ECk_EnableDisable::Enable"))
    float _AnimationPeriod = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_EnableDisable _BoxFilterDownsample = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_EnableDisable _StabilizeGrid = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_Usf_PaletteMode _PaletteMode = ECk_Usf_PaletteMode::ColorSteps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 64,
                      EditCondition = "_PaletteMode == ECk_Usf_PaletteMode::ColorSteps"))
    int32 _ColorSteps = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (EditCondition = "_PaletteMode == ECk_Usf_PaletteMode::CustomPalette"))
    TArray<FLinearColor> _Palette;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_Usf_DitherColorSpace _ColorSpace = ECk_Usf_DitherColorSpace::Gamma;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.1, ClampMin = 0.01, UIMax = 4.0))
    float _PreGamma = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_EnableDisable _Monochrome = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (EditCondition = "_Monochrome == ECk_EnableDisable::Enable"))
    FLinearColor _MonochromeShadowTint = FLinearColor(0.04f, 0.06f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (EditCondition = "_Monochrome == ECk_EnableDisable::Enable"))
    FLinearColor _MonochromeHighlightTint = FLinearColor(0.78f, 0.92f, 0.70f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0, ClampMax = 2.0))
    float _Saturation = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Contrast = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither|Mask")
    FCk_Usf_StylizeMask_Params _Mask;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|ScreenDither")
    ECk_Usf_ScreenDither_DebugMode _DebugMode = ECk_Usf_ScreenDither_DebugMode::Final;

public:
    UFUNCTION(BlueprintPure, Category = "CkUsf|ScreenDither",
              DisplayName = "[Ck][Usf] Get Screen Dither Preset As Params")
    FCk_Usf_ScreenDither_Params
    Get_AsParams() const;
};
