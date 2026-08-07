#pragma once

#include "CoreMinimal.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

// ECk_Usf_HandDrawnStrokePattern is REUSED rather than duplicated: it is the index contract of
// CkUsf_Stylize_StrokePattern's dispatcher in StylizeCommon.ush, and a second enum over the same four
// primitives would be one more thing to keep in sync for nothing. The name says HandDrawn because that
// feature declared it first, not because the patterns belong to it.
#include "CkUsf/Stylize/CkUsf_HandDrawn_Params.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Params.h"

#include "CkUsf_CrossHatch_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// What the strokes are drawn ON. Order is a contract with CrossHatch.ush.
UENUM(BlueprintType)
enum class ECk_Usf_CrossHatchBackground : uint8
{
    Scene,   // keep the rendered frame under the hatching — an overlay
    Paper    // replace it with a flat sheet — a drawing
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CrossHatchBackground);

// --------------------------------------------------------------------------------------------------------------------

// Order is a contract with CrossHatch.ush's DebugMode dispatcher. Every mode past Final bypasses the
// master StyleStrength lerp AND the effect mask — a mask viewed at partial strength is a faint tint,
// not a diagnostic.
UENUM(BlueprintType)
enum class ECk_Usf_CrossHatch_DebugMode : uint8
{
    Final,
    HatchMask,
    HatchDirection,   // RG = the unit direction remapped to 0..1, B = the projected normal's length
    Darkness,
    LayerCoverage,    // how many stroke layers this pixel's darkness has reached
    WorldNormals,
    StylizeMask
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_CrossHatch_DebugMode);

// --------------------------------------------------------------------------------------------------------------------

// Every runtime-drivable setting of the CrossHatch look, in one reflected value. The subsystem owns one
// of these and projects it onto the look's MID; a preset data asset is just an authored instance.
// Defaults ARE the "Sketch" preset — an authored preset that differs from these differs on purpose.
USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_CrossHatch_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Usf_CrossHatch_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

    // One lerp over the whole composite, HandDrawn's shape rather than CelShade's: hatching and the
    // sheet it sits on are one medium, so there is no state where the ink is at full weight and the
    // paper is absent. The whole-look passthrough is still the subsystem's Enabled.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StyleStrength = 1.0f;

    // ---- Direction ----

    // Off (the default) transforms the scene normal into VIEW space first, so the hatch direction is a
    // property of how the surface faces the camera and the lines keep wrapping a form as the camera
    // orbits — the engraving behaviour. On uses the world normal raw, so the direction encodes world
    // orientation instead and re-orients as the camera moves. An artistic choice, not a correctness one.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _UseWorldSpaceNormals = ECk_EnableDisable::Disable;

    // Base direction of layer 0, in degrees. It is also the FALLBACK for pixels whose normal projects to
    // nothing (a surface facing exactly at or away from the camera has no screen direction to give).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = -180.0, ClampMin = -360.0, UIMax = 180.0, ClampMax = 360.0))
    float _AngleOffset = 45.0f;

    // How much of the projected normal's angle is added to the base. 0 = a fixed screen-space hatch that
    // ignores the form entirely (the control that proves the alignment is doing anything); 1 = fully
    // form-following.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _NormalAlignment = 1.0f;

    // ---- Strokes ----

    // Stroke cell size in pixels of the post-process input's viewport.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 2.0, ClampMin = 0.5, UIMax = 64.0))
    float _Spacing = 8.0f;

    // How many stroke fields may accumulate as a pixel darkens. Each owns a slice of the darkness range
    // and is rotated a further _LayerAngleStep, so darker regions gain CROSSING fields rather than
    // thicker lines — which is how the medium actually works.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 4, ClampMax = 4))
    int32 _LayerCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = -180.0, UIMax = 180.0, ClampMax = 180.0))
    float _LayerAngleStep = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_HandDrawnStrokePattern _StrokePattern = ECk_Usf_HandDrawnStrokePattern::DiagonalPencil;

    // Ceiling on how much of a cell a stroke may cover at full darkness. Below 1 the darkest regions
    // keep some paper showing through, which is what stops a drawing turning into a black fill.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeThickness = 0.55f;

    // How far the ideal stroke lattice is allowed to wander — the whole difference between a printed
    // screen and a drawn one.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 1.0, ClampMax = 1.0))
    float _StrokeIrregularity = 0.35f;

    // ---- Tone ----

    // Shifts the whole darkness ramp: positive hatches more of the frame, negative less.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = -0.5, ClampMin = -1.0, UIMax = 0.5, ClampMax = 1.0))
    float _DarknessBias = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 3.0))
    float _DarknessContrast = 1.2f;

    // ---- Background ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_CrossHatchBackground _BackgroundMode = ECk_Usf_CrossHatchBackground::Paper;

    // Authored in SCENE-REFERRED LINEAR, because this look sits pre-tonemap. A value of 1.0 tonemaps to
    // a mid grey, not to paper white — the shipped presets sit well above 1 for that reason.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_BackgroundMode == ECk_Usf_CrossHatchBackground::Paper"))
    FLinearColor _PaperColor = FLinearColor(2.20f, 2.14f, 1.98f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _InkColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0.0, ClampMin = 0.0, UIMax = 2.0, ClampMax = 2.0))
    float _Saturation = 1.0f;

    // ---- Sky ----

    // The sky has no form for the hatch direction to follow, so its lines take whatever angle the sky's
    // normal happens to give — the most obviously wrong thing this look can do. Hence a switch rather
    // than a threshold.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AffectSky = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1000.0, ClampMin = 1.0))
    float _SkyDistance = 100000.0f;

    // ---- Effect mask ----

    // Confines the whole look to (or away from) a Custom-Stencil range. This look is pre-TAA, so its
    // mask edge is temporally resolved like any other geometry edge.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Usf_StylizeMask_Params _Mask;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Usf_CrossHatch_DebugMode _DebugMode = ECk_Usf_CrossHatch_DebugMode::Final;

public:
    CK_PROPERTY(_Enabled);
    CK_PROPERTY(_StyleStrength);
    CK_PROPERTY(_UseWorldSpaceNormals);
    CK_PROPERTY(_AngleOffset);
    CK_PROPERTY(_NormalAlignment);
    CK_PROPERTY(_Spacing);
    CK_PROPERTY(_LayerCount);
    CK_PROPERTY(_LayerAngleStep);
    CK_PROPERTY(_StrokePattern);
    CK_PROPERTY(_StrokeThickness);
    CK_PROPERTY(_StrokeIrregularity);
    CK_PROPERTY(_DarknessBias);
    CK_PROPERTY(_DarknessContrast);
    CK_PROPERTY(_BackgroundMode);
    CK_PROPERTY(_PaperColor);
    CK_PROPERTY(_InkColor);
    CK_PROPERTY(_Saturation);
    CK_PROPERTY(_AffectSky);
    CK_PROPERTY(_SkyDistance);
    CK_PROPERTY(_Mask);
    CK_PROPERTY(_DebugMode);

public:
    auto operator==(const ThisType& InOther) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
