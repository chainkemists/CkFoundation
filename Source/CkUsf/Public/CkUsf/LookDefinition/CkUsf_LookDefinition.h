#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkUsf_LookDefinition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Usf_Domain : uint8
{
    SurfaceLit,
    SurfaceUnlit,
    PostProcess,
    UI,
    Decal
};

// Blend-mode override on top of the domain default. `Inherit` keeps the domain's blend.
UENUM(BlueprintType)
enum class ECk_Usf_BlendMode : uint8
{
    Inherit,
    Opaque,
    Masked,
    Translucent,
    Additive,
    Modulate
};

// Shading-model override. `Inherit` keeps the domain default (SurfaceLit→DefaultLit, others→Unlit).
// Each exotic model is wired together with its required G-buffer outputs (see the generator):
//   Subsurface → SubsurfaceColor (+ Opacity drives scatter); ClearCoat → ClearCoat + ClearCoatRoughness.
UENUM(BlueprintType)
enum class ECk_Usf_ShadingModel : uint8
{
    Inherit,
    Unlit,
    DefaultLit,
    Subsurface,
    ClearCoat
};

UENUM(BlueprintType)
enum class ECk_Usf_ParamType : uint8
{
    Scalar,
    Vector,
    Texture2D,
    TextureCube
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_ParamDesc
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _Name = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_ParamType _Type = ECk_Usf_ParamType::Scalar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    float _DefaultScalar = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FLinearColor _DefaultVector = FLinearColor::Black;

    // Scalar-only: source this param from per-instance custom data (ISM/CkIsmRenderer) instead of a uniform.
    // The generator assigns each per-instance scalar param a 0-based slot in declaration order and wires a
    // PerInstanceCustomData node (DataIndex=slot, ConstDefaultValue=_DefaultScalar). On a non-instanced mesh
    // the node returns the const default, so the look is safe everywhere. The runtime writer must use the
    // SAME slot index (CkIsmRenderer SetCustomDataValueById). Vector per-instance is a follow-up (3 slots).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _PerInstance = false;

    // Object path for Texture2D / TextureCube params, e.g.
    // "/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FString _DefaultTexturePath;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKUSF_API UCkUsf_LookDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Include path resolvable from a Custom node, e.g. "/CkUsf/Looks/Hologram.ush"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FString _UshIncludePath;

    // HLSL function name inside the include, e.g. "CkUsf_Look_Hologram"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _UshFunctionName = NAME_None;

    // Optional WorldPositionOffset entry point (vertex shader), e.g. "CkUsf_Look_Displace_WPO".
    // None = no WPO. Takes FCkUsf_VertexInput + the same params as the pixel fn, returns a world-space offset.
    // Surface domains only; wired into a separate VS-safe Custom node (the pixel node reads pixel-only inputs).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _WpoFunctionName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_Domain _Domain = ECk_Usf_Domain::SurfaceLit;

    // Surface-domain overrides (ignored for PostProcess/UI/Decal, which keep their domain config).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_BlendMode _BlendMode = ECk_Usf_BlendMode::Inherit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_ShadingModel _ShadingModel = ECk_Usf_ShadingModel::Inherit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _TwoSided = false;

    // Usage flags baked into the generated master at generation time. Hand-set
    // flags on a generated master are wiped by the next regeneration, and a
    // missing flag falls back to the default material in packaged builds — so
    // any look rendered through CkIsm needs InstancedStaticMeshes, and any look
    // rendered through CkIskm needs SkeletalMesh (+ MorphTargets if the mesh
    // animates morphs).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithInstancedStaticMeshes = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithSkeletalMesh = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithMorphTargets = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FCk_Usf_ParamDesc> _Parameters;

    // Logical look name; defaults to asset name if None. Drives the generated master path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _LookName = NAME_None;

    UFUNCTION(BlueprintCallable, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Effective Look Name")
    FName Get_EffectiveLookName() const;
};
