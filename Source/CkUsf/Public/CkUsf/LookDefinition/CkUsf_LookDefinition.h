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
    TextureCube,
    Texture2DArray
};

// PostProcess-only: which scene textures a look's Custom node receives (and thereby declares usage for,
// legalizing raw SceneTextureLookup() in its .ush). EMPTY = the default trio (SceneColor/Depth/Normal).
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
// EBlendableLocation). Custom Depth/Stencil-derived looks REQUIRE a pre-TAA location or their edges
// shimmer; pre-TAA is also pre-tonemap. Trade-offs: CkUsf/CLAUDE.md § Blendable location.
UENUM(BlueprintType)
enum class ECk_Usf_BlendableLocation : uint8
{
    AfterTonemapping,      // BL_SceneColorAfterTonemapping — display res, post-TAA (historical default)
    SceneColorAfterDOF,    // BL_SceneColorAfterDOF         — render res, pre-TSR/TAA
    SceneColorBeforeDOF,   // BL_SceneColorBeforeDOF        — render res, pre-TSR/TAA, before DOF
    SceneColorBeforeBloom  // BL_SceneColorBeforeBloom      — display res, post-TAA, pre-bloom/tonemap
};

// Translucency lighting for LIT translucent-family surface looks. `Inherit` keeps the engine default
// (volumetric non-directional — cheap but flat); glass-like surfaces usually want `SurfacePerPixel`.
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
    // uniform. Safe on non-instanced meshes (the node returns the param default). Runtime writers must NOT
    // count slots by hand — query Get_PerInstanceSlotOf / Get_NumPerInstanceFloats.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _PerInstance = false;

    // Per-instance params only: explicit first slot for this param's per-instance data; -1 keeps the auto
    // layout (declaration order). Set it when the instance custom-data layout is owned elsewhere. Explicit
    // slots do NOT advance the auto counter; Get_PerInstanceSlotOf resolves both kinds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    int32 _PerInstanceSlot = -1;

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

    // Optional WorldPositionOffset entry point (vertex shader), e.g. "CkUsf_Look_Displace_WPO"; None = no WPO.
    // Surface domains only. Takes FCkUsf_VertexInput + the same params as the pixel fn, returns a world offset.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _WpoFunctionName = NAME_None;

    // Surface-only: wire mesh TexCoord1/TexCoord2 into the PIXEL Custom node (In.UV1/In.UV2). Opt-in
    // because every wired coordinate costs interpolators on the look's master. The WPO node receives
    // UV1/UV2 unconditionally; this flag concerns the pixel node only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _PixelDataChannels = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_Domain _Domain = ECk_Usf_Domain::SurfaceLit;

    // PostProcess-only (see ECk_Usf_SceneTexture): EMPTY = the default trio; opt into CustomDepth/
    // CustomStencil explicitly (e.g. the SolidOutline look). Ignored for non-PostProcess domains.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<ECk_Usf_SceneTexture> _SceneTextures;

    // PostProcess-only: chain placement of the generated blendable (see the enum for the pre- vs post-TAA
    // trade-off — Custom Depth/Stencil looks need a pre-TAA location). Ignored for non-PostProcess domains.
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
    // "NAME=VALUE". A define change requires regenerating the master.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FString> _Defines;

    // Usage flags baked into the generated master at generation time; hand-set flags on a generated master
    // are wiped by the next regeneration, and a missing flag falls back to the default material in packaged
    // builds. CkIsm needs InstancedStaticMeshes; CkIskm needs SkeletalMesh (+ MorphTargets if morphed).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithInstancedStaticMeshes = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithSkeletalMesh = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithMorphTargets = false;

    // Niagara SPRITE renderers only (MATUSAGE_NiagaraSprites). Without it a sprite renderer using this
    // master falls back to the default material in a packaged build. Ribbon / mesh-particle usages are
    // deliberately absent — add them when a look actually needs one, not speculatively.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _UsedWithNiagaraSprites = false;

    // Surface-only: wire the renderer's per-particle color into the PIXEL Custom node (In.ParticleColor).
    // Opt-in because it costs an interpolator and reads as opaque white outside a particle renderer.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _ParticleColor = false;

    // Surface-only: wire the Niagara Dynamic Material Parameter float4 into the PIXEL Custom node
    // (In.DynamicParameter). The emitter writes it as Particles.DynamicMaterialParameter; channel meanings
    // are the effect's own contract. Opt-in for the same reason as _ParticleColor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    bool _ParticleDynamicParameter = false;

    // Names for the four DynamicParameter channels, purely for readability in the generated master
    // (the .ush reads In.DynamicParameter.xyzw positionally). Empty entries fall back to Param1..4.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FString> _ParticleDynamicParameterNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FCk_Usf_ParamDesc> _Parameters;

    // Logical look name; defaults to asset name if None. Drives the generated master path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _LookName = NAME_None;

    UFUNCTION(BlueprintCallable, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Effective Look Name")
    FName Get_EffectiveLookName() const;

    // First slot of InParamName's per-instance data, or -1 if the param is not per-instance.
    UFUNCTION(BlueprintPure, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Per-Instance Slot Of")
    int32 Get_PerInstanceSlotOf(FName InParamName) const;

    // Total per-instance floats this look consumes (the NumCustomDataFloats an ISM must allocate).
    UFUNCTION(BlueprintPure, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Num Per-Instance Floats")
    int32 Get_NumPerInstanceFloats() const;
};
