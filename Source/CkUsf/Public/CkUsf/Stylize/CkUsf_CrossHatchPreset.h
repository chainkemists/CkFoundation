#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkUsf/Stylize/CkUsf_CrossHatch_Params.h"

#include "CkUsf_CrossHatchPreset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// An authored CrossHatch style (authorable in AngelScript via
// `asset DA_Foo of UCkUsf_CrossHatchPreset {...}`, or as an editor data asset). Public fields mirror
// FCk_Usf_CrossHatch_Params one for one — the UCkUsf_OutlinePreset precedent — and Get_AsParams packs
// them into the value the subsystem actually owns.
UCLASS(BlueprintType)
class CKUSF_API UCkUsf_CrossHatchPreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch")
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StyleStrength = 1.0f;

    // ---- Direction ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Direction")
    ECk_EnableDisable _UseWorldSpaceNormals = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Direction",
              meta = (UIMin = -180.0, ClampMin = -360.0, UIMax = 180.0, ClampMax = 360.0))
    float _AngleOffset = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Direction",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalAlignment = 1.0f;

    // ---- Strokes ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes",
              meta = (UIMin = 2.0, ClampMin = 0.5, UIMax = 64.0))
    float _Spacing = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes",
              meta = (UIMin = 1, ClampMin = 1, UIMax = 4, ClampMax = 4))
    int32 _LayerCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes",
              meta = (UIMin = 0.0, ClampMin = -180.0, UIMax = 180.0, ClampMax = 180.0))
    float _LayerAngleStep = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes")
    ECk_Usf_HandDrawnStrokePattern _StrokePattern = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeThickness = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Strokes",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeIrregularity = 0.35f;

    // ---- Tone ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Tone",
              meta = (UIMin = -0.5, ClampMin = -1.0, UIMax = 0.5, ClampMax = 1.0))
    float _DarknessBias = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Tone",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 3.0))
    float _DarknessContrast = 1.2f;

    // ---- Background ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Background")
    ECk_Usf_CrossHatchBackground _BackgroundMode = ECk_Usf_CrossHatchBackground::Paper;

    // Scene-referred linear — this look is pre-tonemap, so paper white sits well above 1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Background",
              meta = (EditCondition = "_BackgroundMode == ECk_Usf_CrossHatchBackground::Paper"))
    FLinearColor _PaperColor = FLinearColor(2.20f, 2.14f, 1.98f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Background")
    FLinearColor _InkColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Background",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0, ClampMax = 2.0))
    float _Saturation = 1.0f;

    // ---- Sky ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Sky")
    ECk_EnableDisable _AffectSky = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Sky",
              meta = (UIMin = 1000.0, ClampMin = 1.0))
    float _SkyDistance = 100000.0f;

    // ---- Effect mask ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch|Mask")
    FCk_Usf_StylizeMask_Params _Mask;

    // ---- Debug ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CrossHatch")
    ECk_Usf_CrossHatch_DebugMode _DebugMode = ECk_Usf_CrossHatch_DebugMode::Final;

public:
    UFUNCTION(BlueprintPure, Category = "CkUsf|CrossHatch",
              DisplayName = "[Ck][Usf] Get Cross Hatch Preset As Params")
    FCk_Usf_CrossHatch_Params
    Get_AsParams() const;
};

// --------------------------------------------------------------------------------------------------------------------
