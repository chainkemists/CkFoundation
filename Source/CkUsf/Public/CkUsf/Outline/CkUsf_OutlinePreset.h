#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkUsf_OutlinePreset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How the outline behaves with respect to occlusion / fill, mirroring the reference SolidOutlineSystem.
//   Normal           : silhouette + fill only where the object is actually visible (hidden behind walls).
//   SeeThrough       : silhouette + fill always visible, even when the object is occluded.
//   MaskedSeeThrough : see-through, but the fill is stippled into a screen-space mask pattern.
UENUM(BlueprintType)
enum class ECk_Usf_OutlineType : uint8
{
    Normal,
    SeeThrough,
    MaskedSeeThrough
};

// --------------------------------------------------------------------------------------------------------------------

// Data-driven outline style. Authorable in AngelScript via `asset DA_Foo of UCkUsf_OutlinePreset {...}` and
// in the editor. UCkUsf_OutlineSubsystem assigns each ACTIVE preset a Custom-Stencil value and packs these
// fields into a row of the params LUT bound to the SolidOutline post-process look (see SolidOutline.ush).
UCLASS(BlueprintType)
class CKUSF_API UCkUsf_OutlinePreset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline")
    ECk_Usf_OutlineType _OutlineType = ECk_Usf_OutlineType::Normal;

    // Base outline (ring) color. Multiplied by _OutlineBrightness before packing into the LUT.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline")
    FLinearColor _OutlineColor = FLinearColor(0.1f, 0.6f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline", meta = (UIMin = 0.0, ClampMin = 0.0))
    float _OutlineBrightness = 1.0f;

    // Per-preset multiplier on the subsystem's global outline thickness.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline", meta = (UIMin = 0.0, ClampMin = 0.0))
    float _ThicknessScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline")
    bool _FillEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline", meta = (EditCondition = "_FillEnabled"))
    FLinearColor _FillColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline",
              meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0, EditCondition = "_FillEnabled"))
    float _FillOpacity = 0.25f;

    // v1: when set, the interior samples the subsystem's single shared fill texture instead of a flat fill
    // color. Per-preset fill textures are a documented follow-up (would need a texture atlas/array).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf|Outline", meta = (EditCondition = "_FillEnabled"))
    bool _UseFillTexture = false;
};
