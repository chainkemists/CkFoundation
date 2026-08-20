#pragma once

#include "CoreMinimal.h"
#include <Engine/DataAsset.h>

#include "CkPixelArt/CkPixelArt_Params.h"

#include "CkPixelArt_Preset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// An authored pixel-art style (authorable in AngelScript via `asset DA_Foo of UCkPixelArt_Preset {...}`, or as an
// editor data asset).
//
// Public fields mirror FCk_PixelArt_Params one for one rather than nesting it, following UCkUsf_CelShadePreset.
// That is not stylistic: an AngelScript `asset` block assigns reflected properties by name and cannot reach into
// a nested struct, so a preset built around one `_Params` member would be authorable only in the editor.
// Get_AsParams packs these back into the value the subsystem owns.
UCLASS(BlueprintType)
class CKPIXELART_API UCkPixelArt_Preset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer")
    ECk_PixelArt_ResolutionMode _ResolutionMode = ECk_PixelArt_ResolutionMode::FixedHeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer",
              meta = (UIMin = 90, ClampMin = 90, UIMax = 1080, ClampMax = 1080))
    int32 _InternalHeight = 360;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer",
              meta = (UIMin = 1, ClampMin = 1, UIMax = 16, ClampMax = 16))
    int32 _TexelsPerPixel = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer",
              meta = (UIMin = 0, ClampMin = 0, UIMax = 8, ClampMax = 8))
    int32 _MarginTexels = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer")
    ECk_EnableDisable _SnapEnabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Renderer")
    ECk_PixelArt_UpscaleFilter _UpscaleFilter = ECk_PixelArt_UpscaleFilter::BoxFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look")
    ECk_EnableDisable _ApplyLook = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline")
    ECk_EnableDisable _EnableOutline = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline",
              meta = (UIMin = 0.001, ClampMin = 0.0001, UIMax = 2.0))
    float _DepthThreshold = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _AngleZCutoff = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 64.0))
    float _AngleZScale = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalSmoothLow = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalSmoothHigh = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Edges",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _LineDarken = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Edges",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _CreaseBrighten = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Edges")
    ECk_PixelArt_EdgeMode _EdgeMode = ECk_PixelArt_EdgeMode::BandShift;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Banding",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _Bands = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Banding",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _ThresholdGradientSize = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Banding",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _DitherStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Banding")
    ECk_PixelArt_ColorSpace _ColorSpace = ECk_PixelArt_ColorSpace::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Banding",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 0.2, ClampMax = 0.5))
    float _WarmShift = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    ECk_PixelArt_PaletteMode _PaletteMode = ECk_PixelArt_PaletteMode::ColorSteps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 32))
    int32 _PaletteCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor0 = FLinearColor{0.05f, 0.05f, 0.09f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor1 = FLinearColor{0.14f, 0.12f, 0.24f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor2 = FLinearColor{0.28f, 0.20f, 0.33f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor3 = FLinearColor{0.45f, 0.29f, 0.35f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor4 = FLinearColor{0.63f, 0.42f, 0.36f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor5 = FLinearColor{0.80f, 0.59f, 0.42f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor6 = FLinearColor{0.92f, 0.78f, 0.57f, 1.0f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkPixelArt|Look|Palette")
    FLinearColor _PaletteColor7 = FLinearColor{0.99f, 0.95f, 0.85f, 1.0f};

public:
    UFUNCTION(BlueprintPure, Category = "CkPixelArt",
              DisplayName = "[Ck][PixelArt] Get Preset As Params")
    FCk_PixelArt_Params
    Get_AsParams() const;
};
