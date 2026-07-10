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

// PostProcess-only: which scene textures a look's Custom node receives (and thereby declares usage for,
// legalizing raw SceneTextureLookup() in the look's .ush). Maps to ESceneTextureId in the generator.
// An EMPTY _SceneTextures on a PostProcess look means the historical default trio (SceneColor/SceneDepth/
// SceneNormal), so existing looks are unaffected.
UENUM(BlueprintType)
enum class ECk_Usf_SceneTexture : uint8
{
    SceneColor,     // PPI_PostProcessInput0  -> In.SceneColor
    SceneDepth,     // PPI_SceneDepth         -> In.SceneDepth
    SceneNormal,    // PPI_WorldNormal        -> In.SceneNormal
    CustomDepth,    // PPI_CustomDepth        -> In.CustomDepth
    CustomStencil   // PPI_CustomStencil      -> In.CustomStencil
};

// PostProcess-only: where in the post-processing chain the generated blendable runs (maps to
// EBlendableLocation). The pre-TAA locations (SceneColorAfterDOF/SceneColorBeforeDOF) run at rendering
// resolution BEFORE TSR/TAA, so the look's output is temporally accumulated like ordinary geometry —
// required for anything derived from Custom Depth/Stencil (those buffers are rendered with the
// TAA-jittered projection every frame; a look placed after tonemapping thresholds that jittered mask
// with no temporal resolve ever seeing it, so its edges shimmer even on a stationary camera).
// Trade-off: pre-TAA locations are also pre-tonemap — output colors are scene-referred linear (the
// tonemapper remaps them and bloom sees them), and TSR may slightly ghost the output behind fast movers.
UENUM(BlueprintType)
enum class ECk_Usf_BlendableLocation : uint8
{
    AfterTonemapping,      // BL_SceneColorAfterTonemapping — display res, post-TAA (historical default)
    SceneColorAfterDOF,    // BL_SceneColorAfterDOF         — render res, pre-TSR/TAA
    SceneColorBeforeDOF,   // BL_SceneColorBeforeDOF        — render res, pre-TSR/TAA, before DOF
    SceneColorBeforeBloom  // BL_SceneColorBeforeBloom      — display res, post-TAA, pre-bloom/tonemap
};

// Translucency lighting for LIT translucent-family surface looks. `Inherit` keeps the engine default
// (volumetric non-directional — cheap but flat). Glass-like surfaces usually want `SurfacePerPixel`
// (forward per-pixel lighting). Ignored unless the look resolves to a lit, translucent-family blend.
UENUM(BlueprintType)
enum class ECk_Usf_TranslucencyLighting : uint8
{
    Inherit,
    VolumetricNonDirectional,
    VolumetricDirectional,
    Surface,
    SurfacePerPixel
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

    // Scalar/Vector-only: source this param from per-instance custom data (ISM/CkIsmRenderer) instead of a
    // uniform. Slots are assigned in declaration order — a Scalar takes 1 float, a Vector takes 3 (rgb) —
    // and the generator wires a PerInstanceCustomData(3Vector) node (DataIndex=slot, ConstDefaultValue=the
    // param default). On a non-instanced mesh the node returns the const default, so the look is safe
    // everywhere. Runtime writers must NOT count slots by hand — query Get_PerInstanceSlotOf/
    // Get_NumPerInstanceFloats on the LookDefinition (same walk the generator uses).
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

    // Surface-only: wire mesh TexCoord1/TexCoord2 into the PIXEL Custom node (In.UV1/In.UV2).
    // Opt-in because every wired coordinate costs interpolators on the look's master — only looks
    // whose PIXEL stage decodes mesh data channels need it (e.g. CkVat's normal-texture lookup).
    // The WPO node receives UV1/UV2 unconditionally; this flag concerns the pixel node only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _PixelDataChannels = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_Domain _Domain = ECk_Usf_Domain::SurfaceLit;

    // PostProcess-only: the scene textures this look's Custom node receives. EMPTY = the default trio
    // (SceneColor/SceneDepth/SceneNormal), so existing PostProcess looks need not set this. Set it
    // explicitly to opt into CustomDepth/CustomStencil (e.g. the SolidOutline look). Ignored for non-PostProcess domains.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<ECk_Usf_SceneTexture> _SceneTextures;

    // PostProcess-only: chain placement of the generated blendable (see the enum for the pre- vs
    // post-TAA trade-off). Looks reading Custom Depth/Stencil want a pre-TAA location
    // (e.g. the SolidOutline look). Ignored for non-PostProcess domains.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_BlendableLocation _BlendableLocation = ECk_Usf_BlendableLocation::AfterTonemapping;

    // Surface-domain overrides (ignored for PostProcess/UI/Decal, which keep their domain config).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_BlendMode _BlendMode = ECk_Usf_BlendMode::Inherit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_ShadingModel _ShadingModel = ECk_Usf_ShadingModel::Inherit;

    // Lit translucent-family surface looks only (see the enum). `Inherit` keeps the engine default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_TranslucencyLighting _TranslucencyLighting = ECk_Usf_TranslucencyLighting::Inherit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _TwoSided = false;

    // Preprocessor defines injected into the look's Custom nodes (pixel AND WPO), each "NAME" or
    // "NAME=VALUE" — the static-switch/quality-knob equivalent, e.g. "CKUSF_OUTLINE_AA_RADIUS=2.5"
    // to retune a #ifndef default in the .ush without editing it, or "MYLOOK_HIGH_QUALITY" gating
    // an #ifdef block. A define change requires regenerating the master.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FString> _Defines;

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

    // Per-instance custom-data slot layout — THE source of truth shared by the generator (node
    // DataIndex) and runtime writers (CkIsmRenderer SetCustomDataValueById / NumCustomDataFloats).
    // Slots accrue over _Parameters in declaration order: per-instance Scalar = 1 float,
    // per-instance Vector = 3 floats (rgb). Never count slots by hand.

    // First slot of InParamName's per-instance data, or -1 if the param is not per-instance.
    UFUNCTION(BlueprintPure, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Per-Instance Slot Of")
    int32 Get_PerInstanceSlotOf(FName InParamName) const;

    // Total per-instance floats this look consumes (the NumCustomDataFloats an ISM must allocate).
    UFUNCTION(BlueprintPure, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Num Per-Instance Floats")
    int32 Get_NumPerInstanceFloats() const;
};
