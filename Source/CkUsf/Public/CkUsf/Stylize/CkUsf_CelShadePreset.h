#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

#include "CkUsf_CelShadePreset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// An authored CelShade style (authorable in AngelScript via `asset DA_Foo of UCkUsf_CelShadePreset {...}`,
// or as an editor data asset). Public fields mirror FCk_Usf_CelShade_Params one for one — the
// UCkUsf_OutlinePreset precedent — and Get_AsParams packs them into the value the subsystem owns.
UCLASS(BlueprintType)
class CKUSF_API UCkUsf_CelShadePreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade")
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _Bands = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 0.05, ClampMin = 0.01, UIMax = 4.0))
    float _Midpoint = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = -1.0, ClampMin = -1.0, UIMax = 1.0, ClampMax = 1.0))
    float _BandOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 0.1, ClampMin = 0.01, UIMax = 4.0))
    float _Distribution = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _BandSoftness = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _ShadowLift = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _Strength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Bands")
    ECk_EnableDisable _QuantizeFinalColor = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern")
    ECk_EnableDisable _EnablePattern = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern")
    ECk_Usf_CelPattern _Pattern = ECk_Usf_CelPattern::RoundDots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern")
    ECk_Usf_CelPatternSpace _PatternSpace = ECk_Usf_CelPatternSpace::World;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _PatternStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 0.1, ClampMin = 0.01, UIMax = 8.0))
    float _PatternContrast = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 200.0))
    float _PatternWorldSize = 16.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 1.0, ClampMin = 0.01, UIMax = 64.0))
    float _PatternPixelSize = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 1.0, ClampMin = 0.1, UIMax = 16.0))
    float _TriplanarSharpness = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _PatternDistanceScaling = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = -4.0, ClampMin = -8.0, UIMax = 8.0))
    float _PatternOctaveMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = -4.0, ClampMin = -8.0, UIMax = 8.0))
    float _PatternOctaveMax = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Pattern",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 32.0))
    float _PatternScrollSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Colour")
    FLinearColor _ShadowTint = FLinearColor(0.72f, 0.78f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Colour")
    FLinearColor _LightTint = FLinearColor(1.0f, 0.98f, 0.92f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Colour",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0))
    float _Saturation = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Colour",
              meta = (UIMin = 0.001, ClampMin = 0.001, UIMax = 0.5))
    float _MinimumAlbedo = 0.04f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Colour")
    ECk_EnableDisable _AffectUnlit = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky")
    ECk_EnableDisable _EnableSky = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky",
              meta = (UIMin = 1000.0, ClampMin = 1.0))
    float _SkyDistance = 100000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _SkyBands = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _SkyStrength = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky")
    ECk_Usf_CelPattern _SkyPattern = ECk_Usf_CelPattern::Bayer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _SkyPatternStrength = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Sky",
              meta = (UIMin = 0.1, ClampMin = 0.01, UIMax = 16.0))
    float _SkyPatternScale = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Metallic",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Metallic",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 16))
    int32 _MetallicBands = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Metallic",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicStrength = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Metallic",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _MetallicPatternStrength = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Specular")
    ECk_EnableDisable _EnableSpecular = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Specular",
              meta = (UIMin = 1, ClampMin = 1, UIMax = 8))
    int32 _SpecularSteps = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Specular",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _SpecularThreshold = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Specular",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0))
    float _SpecularIntensity = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Specular",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _SpecularRoughnessCutoff = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim")
    ECk_EnableDisable _EnableRimLight = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim",
              meta = (UIMin = 0.5, ClampMin = 0.01, UIMax = 16.0))
    float _RimPower = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _RimThreshold = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim",
              meta = (UIMin = 0.01, ClampMin = 0.001, UIMax = 1.0, ClampMax = 1.0))
    float _RimSoftness = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 4.0))
    float _RimIntensity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _RimFollowsLighting = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Rim")
    FLinearColor _RimColor = FLinearColor(1.0f, 0.95f, 0.85f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline")
    ECk_EnableDisable _EnableOutline = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 1.0, ClampMin = 1.0, UIMax = 8.0))
    float _OutlineThickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline")
    ECk_Usf_CelOutlineQuality _OutlineQuality = ECk_Usf_CelOutlineQuality::FourTap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline")
    FLinearColor _OutlineColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline")
    ECk_Usf_CelOutlineBlend _OutlineBlendMode = ECk_Usf_CelOutlineBlend::Blend;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _OutlineOpacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 0.001, ClampMin = 0.0001, UIMax = 2.0))
    float _OutlineDepthThreshold = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 0.01, ClampMin = 0.001, UIMax = 1.0, ClampMax = 1.0))
    float _OutlineNormalThreshold = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 0.01, ClampMin = 0.001, UIMax = 2.0))
    float _OutlineAlbedoThreshold = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Outline",
              meta = (UIMin = 0.0, ClampMin = 0.0))
    float _OutlineDistanceFade = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Stencil")
    ECk_EnableDisable _EnableStencilPatterns = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade|Stencil",
              meta = (UIMin = 2, ClampMin = 2, UIMax = 245))
    int32 _StencilBase = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|CelShade")
    ECk_Usf_CelShade_DebugMode _DebugMode = ECk_Usf_CelShade_DebugMode::Final;

public:
    UFUNCTION(BlueprintPure, Category = "CkUsf|CelShade",
              DisplayName = "[Ck][Usf] Get Cel Shade Preset As Params")
    FCk_Usf_CelShade_Params
    Get_AsParams() const;
};
