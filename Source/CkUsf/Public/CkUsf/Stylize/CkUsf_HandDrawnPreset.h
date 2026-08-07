#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkUsf/Stylize/CkUsf_HandDrawn_Params.h"

#include "CkUsf_HandDrawnPreset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// An authored HandDrawn style (authorable in AngelScript via
// `asset DA_Foo of UCkUsf_HandDrawnPreset {...}`, or as an editor data asset). Public fields mirror
// FCk_Usf_HandDrawn_Params one for one — the UCkUsf_OutlinePreset precedent — and Get_AsParams packs them
// into the value the subsystem actually owns.
UCLASS(BlueprintType)
class CKUSF_API UCkUsf_HandDrawnPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn")
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StyleStrength = 1.0f;

    // ---- Paint ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint")
    ECk_EnableDisable _SimplifyColor = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 16,
                      EditCondition = "_SimplifyColor == ECk_EnableDisable::Enable"))
    int32 _ColorLevels = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0,
                      EditCondition = "_SimplifyColor == ECk_EnableDisable::Enable"))
    float _ColorSoftness = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Saturation = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Contrast = 1.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint")
    FLinearColor _ShadowTint = FLinearColor(0.72f, 0.76f, 0.90f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint")
    FLinearColor _HighlightTint = FLinearColor(1.0f, 0.97f, 0.90f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _TintStrength = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint")
    ECk_EnableDisable _AffectSky = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paint",
              meta = (UIMin = 1000.0, ClampMin = 1.0))
    float _SkyDistance = 100000.0f;

    // ---- Ink ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink")
    ECk_EnableDisable _EnableInk = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink")
    FLinearColor _InkColor = FLinearColor(0.05f, 0.04f, 0.06f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 1.0, ClampMin = 1.0, UIMax = 8.0))
    float _InkThickness = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _InkOpacity = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _DepthThreshold = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _NormalThreshold = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.01, ClampMin = 0.0001, UIMax = 1.0))
    float _ColorEdgeThreshold = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0))
    float _LineVariation = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 256.0))
    float _LineScale = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.0, ClampMin = 0.0))
    float _InkFadeStartDistance = 0.0f;

    // 0 disables the fade; any other value must be greater than the start, or the subsystem rejects the
    // whole settings value.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Ink",
              meta = (UIMin = 0.0, ClampMin = 0.0))
    float _InkFadeEndDistance = 0.0f;

    // ---- Shadow strokes ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes")
    ECk_EnableDisable _EnableShadowStrokes = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes")
    ECk_Usf_HandDrawnStrokePattern _StrokePattern = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes")
    ECk_Usf_HandDrawnStrokeSpace _StrokeSpace = ECk_Usf_HandDrawnStrokeSpace::ScreenStable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeStrength = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeShadowThreshold = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 64.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::ScreenStable"))
    float _StrokePixelSize = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 256.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::WorldAttached"))
    float _StrokeWorldSize = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeIrregularity = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Strokes",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 16.0,
                      EditCondition = "_StrokeSpace == ECk_Usf_HandDrawnStrokeSpace::WorldAttached"))
    float _StrokeTriplanarSharpness = 4.0f;

    // ---- Paper ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paper")
    ECk_EnableDisable _EnablePaper = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paper",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _GrainStrength = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paper",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 32.0))
    float _GrainScale = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paper",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _FiberStrength = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn|Paper",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _PaperWarmth = 0.5f;

    // ---- Debug ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|HandDrawn")
    ECk_Usf_HandDrawn_DebugMode _DebugMode = ECk_Usf_HandDrawn_DebugMode::FinalImage;

public:
    UFUNCTION(BlueprintPure, Category = "CkUsf|HandDrawn",
              DisplayName = "[Ck][Usf] Get Hand Drawn Preset As Params")
    FCk_Usf_HandDrawn_Params
    Get_AsParams() const;
};
