#pragma once

#include "CoreMinimal.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "UObject/StrongObjectPtr.h"

#include "CkVatCollection_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class USkeleton;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UAnimSequenceBase;

// --------------------------------------------------------------------------------------------------------------------

// Vertex: per-vertex position/normal texel per frame — cheapest, vertex count bounded by texture width.
// Bone:   per-bone position/rotation texel per frame + mesh-carried indices/weights — scales to
//         high-vertex meshes; textures shareable across meshes on the same skeleton.
UENUM(BlueprintType)
enum class ECk_Vat_BakeMode : uint8
{
    Vertex,
    Bone
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_BakeMode);

// --------------------------------------------------------------------------------------------------------------------

// Ultra = RGBA32F (full float; large VRAM, for extreme bounds/precision needs); High = PF_FloatRGBA
// (16f); Low = RGBA8 with bounds-normalized values (cheapest VRAM, visible quantization on large motions).
UENUM(BlueprintType)
enum class ECk_Vat_Precision : uint8
{
    High,
    Low,
    Ultra
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_Precision);

// --------------------------------------------------------------------------------------------------------------------

// Bone mode: how many bone influences each vertex keeps (strongest-N, weights renormalized).
// The shader skips zero-weight influences, so this is a bake-side knob with no shader variant.
UENUM(BlueprintType)
enum class ECk_Vat_BoneInfluences : uint8
{
    One,
    Two,
    Four
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_BoneInfluences);

// --------------------------------------------------------------------------------------------------------------------

// Bone mode: where per-vertex bone indices/weights live.
// MeshChannels:  indices in two UV channels + weights in vertex color (sRGB-pre-decoded).
// WeightTexture: indices + weights in two data textures addressed by one lookup UV.
// Trade-offs: CkVat/CLAUDE.md.
UENUM(BlueprintType)
enum class ECk_Vat_BoneWeightStorage : uint8
{
    MeshChannels,
    WeightTexture
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_BoneWeightStorage);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVAT_API FCk_VatCollection_ClipDef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VatCollection_ClipDef);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimSequenceBase> _Sequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Name;

public:
    CK_PROPERTY_GET(_Sequence);
    CK_PROPERTY_GET(_Name);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VatCollection_ClipDef, _Sequence, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

// One baked clip's slot in the texture frame layout — written by the CkVatEditor baker, SERIALIZED
// on the collection asset.
USTRUCT(BlueprintType)
struct CKVAT_API FCk_Vat_BakedClip
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vat_BakedClip);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _Name;

    // Texture row of this clip's local frame 0 (row 0 is the reference pose).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _FrameIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _FrameCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _SampleFrequency = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Time _PlayLength;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_FrameIndex);
    CK_PROPERTY_GET(_FrameCount);
    CK_PROPERTY_GET(_SampleFrequency);
    CK_PROPERTY_GET(_PlayLength);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Vat_BakedClip, _Name, _FrameIndex, _FrameCount, _SampleFrequency, _PlayLength);
};

// --------------------------------------------------------------------------------------------------------------------

// Every bake KNOB in one value-struct (the skeleton/mesh/clip identity stays on the collection).
USTRUCT(BlueprintType)
struct CKVAT_API FCk_Vat_BakeSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vat_BakeSettings);

private:
    // Frames-per-second each clip is sampled at. Each frame = one texture row. Higher = smoother + more VRAM.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 120, ClampMax = 120))
    int32 _SampleFrequency = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_BakeMode _BakeMode = ECk_Vat_BakeMode::Vertex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_Precision _Precision = ECk_Vat_Precision::High;

    // Bone mode only: strongest-N influence truncation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_BoneInfluences _BoneInfluences = ECk_Vat_BoneInfluences::Four;

    // Bone mode only: indices/weights carrier (see the enum for the trade-offs).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_BoneWeightStorage _BoneWeightStorage = ECk_Vat_BoneWeightStorage::MeshChannels;

    // Which source-model LOD the bake reads (positions, skinning, the baked static mesh). Higher
    // LODs bring Vertex mode under its texture-width vertex cap on dense meshes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0, UIMax = 7, ClampMax = 7))
    int32 _SourceLOD = 0;

    // UV channel index on the baked static mesh that carries each vertex's texture-lookup coordinate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 7, ClampMax = 7))
    int32 _LookupUVChannel = 1;

    // Texture-size budgets the bake must fit (ensures fire otherwise). Vertex mode: width = vertex
    // count; Bone mode: rows also bound the clip-frame total.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 64, ClampMin = 64, UIMax = 16384, ClampMax = 16384))
    int32 _MaxTextureWidth = 4096;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 64, ClampMin = 64, UIMax = 16384, ClampMax = 16384))
    int32 _MaxTextureRows = 8192;

    // Extract root motion while sampling poses: root-motion clips bake in place instead of baking
    // their travel into the texture (the gameplay owner moves the instance).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _ExtractRootMotion = false;

    // Sample the raw authored tracks, skipping skeleton retargeting.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _DisableRetargeting = false;

public:
    CK_PROPERTY(_SampleFrequency);
    CK_PROPERTY(_BakeMode);
    CK_PROPERTY(_Precision);
    CK_PROPERTY(_BoneInfluences);
    CK_PROPERTY(_BoneWeightStorage);
    CK_PROPERTY(_SourceLOD);
    CK_PROPERTY(_LookupUVChannel);
    CK_PROPERTY(_MaxTextureWidth);
    CK_PROPERTY(_MaxTextureRows);
    CK_PROPERTY(_ExtractRootMotion);
    CK_PROPERTY(_DisableRetargeting);
};

// --------------------------------------------------------------------------------------------------------------------

// Every bake OUTPUT in one value-struct — written ONLY by ApplyBakeResults; read-only everywhere else.
USTRUCT(BlueprintType)
struct CKVAT_API FCk_Vat_BakedData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Vat_BakedData);

private:
    friend class UCk_VatCollection_Data;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _BakedMesh;

    // Vertex mode: per-vertex component-space position offsets from the bind pose, one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _PositionTexture;

    // Vertex mode: skinned normals in each vertex's BIND-POSE TANGENT frame, one row per frame.
    // Tangent-space is deliberate — invariant under the per-instance transform (CkVat/CLAUDE.md).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _NormalTexture;

    // Bone mode: per-bone component-space positions (relative to ref pose), one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BonePositionTexture;

    // Bone mode: per-bone rotations (quaternions), one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BoneRotationTexture;

    // Bone mode + WeightTexture storage: per-vertex render-bone indices (RGBA16F, raw floats —
    // exact for index magnitudes) addressed by the mesh's lookup UV.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BoneIndexTexture;

    // Bone mode + WeightTexture storage: per-vertex weights (all four stored — no derive), same
    // addressing. Linear data end-to-end (no mesh-build color encode on this path).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BoneWeightTexture;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Vat_BakedClip> _BakedClips;

    // Texel dimensions of the VAT textures (width = vertex / render-bone count by mode; rows =
    // total frame rows). SERIALIZED: never derive them from GetSizeX/Y (see CkVat/CLAUDE.md).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _TextureWidth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _TextureRows = 0;

    // Conservative culling bounds covering every baked pose (never smaller than the mesh box).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FBox _AnimatedBounds = FBox(ForceInit);

    // Value range of the position texels. Low precision stores texels normalized into this range; the
    // material decode needs it in BOTH precisions for a uniform shader path.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _PositionBoundsMin = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector _PositionBoundsMax = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _IsBaked = false;

    // Digest of the bake INPUTS captured when the bake ran — Get_IsBakeStale compares it against the
    // current inputs so edits after a bake surface loudly instead of silently playing stale textures.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _BakedInputsHash;

public:
    CK_PROPERTY_GET(_BakedMesh);
    CK_PROPERTY_GET(_PositionTexture);
    CK_PROPERTY_GET(_NormalTexture);
    CK_PROPERTY_GET(_BonePositionTexture);
    CK_PROPERTY_GET(_BoneRotationTexture);
    CK_PROPERTY_GET(_BoneIndexTexture);
    CK_PROPERTY_GET(_BoneWeightTexture);
    CK_PROPERTY_GET(_BakedClips);
    CK_PROPERTY_GET(_TextureWidth);
    CK_PROPERTY_GET(_TextureRows);
    CK_PROPERTY_GET(_AnimatedBounds);
    CK_PROPERTY_GET(_PositionBoundsMin);
    CK_PROPERTY_GET(_PositionBoundsMax);
    CK_PROPERTY_GET(_IsBaked);
    CK_PROPERTY_GET(_BakedInputsHash);
};

// --------------------------------------------------------------------------------------------------------------------

// Vertex-animation-texture collection: the bake INPUTS and OUTPUTS for one skeletal mesh. The bake
// itself lives in CkVatEditor (in-editor only); at runtime this asset is read-only.
UCLASS(BlueprintType)
class CKVAT_API UCk_VatCollection_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    // The baker subsystem's transient-collection factory fills the bake INPUTS programmatically (gyms/tests).
    friend class UCkVat_BakerSubsystem;

public:
    CK_GENERATED_BODY(UCk_VatCollection_Data);

protected:
#if WITH_EDITOR
    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    // ---- bake inputs (identity + grouped knobs) ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeleton> _Skeleton;

    // Source for the baked static mesh, the vertex data, and (Bone mode) the bone indices/weights.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeletalMesh> _SourceMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true, TitleProperty = "{_Name}"))
    TArray<FCk_VatCollection_ClipDef> _Clips;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true, ShowOnlyInnerProperties))
    FCk_Vat_BakeSettings _BakeSettings;

    // ---- rendering (not baked — designer-set) ----

    // Albedo the VAT look samples with UV0 (the runtime replaces the mesh materials with the shared
    // VAT MID; source-material graphs do not carry over — v1 look = BaseColor + VAT deformation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BaseColorTexture;

    // ---- bake outputs (written by the CkVatEditor baker; read-only everywhere else) ----

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true, ShowOnlyInnerProperties))
    FCk_Vat_BakedData _BakedData;

public:
    CK_PROPERTY_GET(_Skeleton);
    CK_PROPERTY_GET(_SourceMesh);
    CK_PROPERTY_GET(_Clips);
    CK_PROPERTY_GET(_BakeSettings);
    CK_PROPERTY_GET(_BaseColorTexture);
    CK_PROPERTY_GET(_BakedData);

public:
    // Index into the SERIALIZED baked clip table (the runtime source of truth), or INDEX_NONE.
    auto
    Find_BakedClipIndex_ByName(FName InClipName) const -> int32;

#if WITH_EDITOR
public:
    // Digest of the CURRENT bake inputs. ApplyBakeResults stamps it into _BakedInputsHash; a
    // mismatch afterwards means the serialized bake no longer matches what the inputs would produce.
    auto
    Compute_BakeInputsHash() const -> FString;

    // True when the collection is baked but its inputs changed since — the bake outputs are stale.
    auto
    Get_IsBakeStale() const -> bool;

    // The CkVatEditor baker's write-back (the ONLY sanctioned mutation path for the Baked category).
    // TStrongObjectPtr, not TObjectPtr: a plain non-USTRUCT struct is NOT GC-traced, so the freshly
    // baked mesh/textures would be collectable between bake and apply.
    struct FCk_Vat_BakeResults
    {
        TStrongObjectPtr<UStaticMesh> BakedMesh;
        TStrongObjectPtr<UTexture2D> PositionTexture;
        TStrongObjectPtr<UTexture2D> NormalTexture;
        TStrongObjectPtr<UTexture2D> BonePositionTexture;
        TStrongObjectPtr<UTexture2D> BoneRotationTexture;
        TStrongObjectPtr<UTexture2D> BoneIndexTexture;
        TStrongObjectPtr<UTexture2D> BoneWeightTexture;
        TArray<FCk_Vat_BakedClip> BakedClips;
        int32 TextureWidth = 0;
        int32 TextureRows = 0;
        FBox AnimatedBounds = FBox(ForceInit);
        FVector PositionBoundsMin = FVector::ZeroVector;
        FVector PositionBoundsMax = FVector::ZeroVector;
    };

    // Returns false (after a loud ensure) when the results are invalid and the collection was
    // NOT stamped as baked — the caller must treat the bake as failed (no package save).
    auto
    ApplyBakeResults(const FCk_Vat_BakeResults& InResults) -> bool;
#endif
};
