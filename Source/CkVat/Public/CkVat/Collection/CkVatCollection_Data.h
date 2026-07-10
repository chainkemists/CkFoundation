#pragma once

#include "CoreMinimal.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkVatCollection_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class USkeleton;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UAnimSequenceBase;

// --------------------------------------------------------------------------------------------------------------------

// Which VAT encoding the bake produces (a collection bakes exactly one).
// Vertex: per-vertex position/normal texel per frame — cheapest playback, no bone data at runtime,
//         vertex count bounded by the texture width; textures are per-mesh.
// Bone:   per-bone position/rotation texel per frame + per-vertex indices/weights carried on the mesh —
//         scales to high-vertex meshes; textures shareable across meshes that skin to the same skeleton.
UENUM(BlueprintType)
enum class ECk_Vat_BakeMode : uint8
{
    Vertex,
    Bone
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_BakeMode);

// --------------------------------------------------------------------------------------------------------------------

// High = PF_FloatRGBA (16f) textures; Low = RGBA8 with bounds-normalized values (cheaper VRAM, visible
// quantization on large motions).
UENUM(BlueprintType)
enum class ECk_Vat_Precision : uint8
{
    High,
    Low
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_Precision);

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

// One baked clip's slot in the texture frame layout. Written by the CkVatEditor baker, SERIALIZED on the
// collection asset (unlike CkIskmRenderer's transient bake, the VAT bake IS the shipped asset — cooked
// builds never re-sample sequences).
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

// Vertex-animation-texture collection: bake INPUTS (skeleton + source skeletal mesh + clip list) and bake
// OUTPUTS (static mesh with a generated lookup-UV channel, VAT textures, serialized clip table). The bake
// itself lives in CkVatEditor (in-editor only); at runtime this asset is read-only.
// Mirrors UCk_IskmAnimCollection_Data's shape where the concerns overlap.
UCLASS(BlueprintType)
class CKVAT_API UCk_VatCollection_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    // The baker subsystem's transient-collection factory fills the bake INPUTS programmatically
    // (gyms/tests); serialized assets are authored in the details panel instead.
    friend class UCkVat_BakerSubsystem;

public:
    CK_GENERATED_BODY(UCk_VatCollection_Data);

protected:
#if WITH_EDITOR
    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    // ---- bake inputs ----

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

    // Frames-per-second each clip is sampled at. Each frame = one texture row. Higher = smoother + more VRAM.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 120, ClampMax = 120))
    int32 _SampleFrequency = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    ECk_Vat_BakeMode _BakeMode = ECk_Vat_BakeMode::Vertex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    ECk_Vat_Precision _Precision = ECk_Vat_Precision::High;

    // UV channel index on the baked static mesh that carries each vertex's texture-lookup coordinate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 7, ClampMax = 7))
    int32 _LookupUVChannel = 1;

    // Extract root motion while sampling poses: root-motion clips bake in place instead of baking
    // their travel into the texture (the gameplay owner moves the instance).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    bool _ExtractRootMotion = false;

    // Sample the raw authored tracks, skipping skeleton retargeting (mirrors the Iskm collection toggle).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bake",
              meta = (AllowPrivateAccess = true))
    bool _DisableRetargeting = false;

    // ---- rendering (not baked — designer-set) ----

    // Albedo the VAT look samples with UV0 (the runtime replaces the mesh materials with the shared
    // VAT MID; source-material graphs do not carry over — v1 look = BaseColor + VAT deformation).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BaseColorTexture;

    // ---- bake outputs (written by the CkVatEditor baker; read-only everywhere else) ----

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _BakedMesh;

    // Vertex mode: per-vertex component-space position offsets from the bind pose, one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _PositionTexture;

    // Vertex mode: skinned normals in each vertex's BIND-POSE TANGENT frame, one row per frame.
    // Tangent-space is deliberate: it is invariant under the per-instance transform (the TBN
    // co-rotates), so the pixel shader feeds the material's tangent-space Normal pin directly —
    // no per-instance basis exists in the PS (only IS_NANITE_PASS exposes InstanceId there).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _NormalTexture;

    // Bone mode: per-bone component-space positions (relative to ref pose), one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BonePositionTexture;

    // Bone mode: per-bone rotations (quaternions), one row per frame.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _BoneRotationTexture;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Vat_BakedClip> _BakedClips;

    // Conservative culling bounds covering every baked pose (never smaller than the mesh box).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    FBox _AnimatedBounds = FBox(ForceInit);

    // Value range of the position texels (vertex-mode offsets / bone-mode translations). Low precision
    // stores texels normalized into this range; the material decode needs it in BOTH precisions for a
    // uniform shader path.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    FVector _PositionBoundsMin = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    FVector _PositionBoundsMax = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    bool _IsBaked = false;

    // Digest of the bake INPUTS captured when the bake ran — Get_IsBakeStale compares it against the
    // current inputs so edits after a bake surface loudly (details-panel Rebake + IsDataValid error)
    // instead of silently playing stale textures.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Baked",
              meta = (AllowPrivateAccess = true))
    FString _BakedInputsHash;

public:
    CK_PROPERTY_GET(_Skeleton);
    CK_PROPERTY_GET(_SourceMesh);
    CK_PROPERTY_GET(_Clips);
    CK_PROPERTY_GET(_SampleFrequency);
    CK_PROPERTY_GET(_BakeMode);
    CK_PROPERTY_GET(_Precision);
    CK_PROPERTY_GET(_LookupUVChannel);
    CK_PROPERTY_GET(_ExtractRootMotion);
    CK_PROPERTY_GET(_DisableRetargeting);
    CK_PROPERTY_GET(_BaseColorTexture);
    CK_PROPERTY_GET(_BakedMesh);
    CK_PROPERTY_GET(_PositionTexture);
    CK_PROPERTY_GET(_NormalTexture);
    CK_PROPERTY_GET(_BonePositionTexture);
    CK_PROPERTY_GET(_BoneRotationTexture);
    CK_PROPERTY_GET(_BakedClips);
    CK_PROPERTY_GET(_AnimatedBounds);
    CK_PROPERTY_GET(_PositionBoundsMin);
    CK_PROPERTY_GET(_PositionBoundsMax);
    CK_PROPERTY_GET(_IsBaked);
    CK_PROPERTY_GET(_BakedInputsHash);

public:
    // Index into the SERIALIZED baked clip table (the runtime source of truth), or INDEX_NONE.
    auto
    Find_BakedClipIndex_ByName(FName InClipName) const -> int32;

#if WITH_EDITOR
public:
    // Digest of the CURRENT bake inputs (skeleton/mesh/clips/sampling settings). ApplyBakeResults
    // stamps it into _BakedInputsHash; a mismatch afterwards means the serialized bake no longer
    // matches what the inputs would produce.
    auto
    Compute_BakeInputsHash() const -> FString;

    // True when the collection is baked but its inputs changed since — the bake outputs are stale.
    auto
    Get_IsBakeStale() const -> bool;

    // The CkVatEditor baker's write-back (the ONLY sanctioned mutation path for the Baked category).
    struct FCk_Vat_BakeResults
    {
        TObjectPtr<UStaticMesh> BakedMesh;
        TObjectPtr<UTexture2D> PositionTexture;
        TObjectPtr<UTexture2D> NormalTexture;
        TObjectPtr<UTexture2D> BonePositionTexture;
        TObjectPtr<UTexture2D> BoneRotationTexture;
        TArray<FCk_Vat_BakedClip> BakedClips;
        FBox AnimatedBounds = FBox(ForceInit);
        FVector PositionBoundsMin = FVector::ZeroVector;
        FVector PositionBoundsMax = FVector::ZeroVector;
    };

    auto
    ApplyBakeResults(const FCk_Vat_BakeResults& InResults) -> void;
#endif
};
