#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "Templates/PimplPtr.h"

#include "CkIskmAnimCollection_Fragment_Data.generated.h"

// Plan-2 transient CPU bone-matrix bake (see CkIskmAnimCollection_BakedPose.h). Forward-declared here so the
// widely-included asset header stays light; the full type is only pulled into the .cpp.
struct FCk_Iskm_BakedPose;

UENUM(BlueprintType)
enum class ECk_IskmAnimCollection_ValidationResult : uint8
{
    Valid,
    SkeletonMissing,
    SequenceSkeletonMismatch,
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmAnimCollection_SequenceDef
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IskmAnimCollection_SequenceDef);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UAnimSequenceBase> _Sequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Name;

public:
    CK_PROPERTY_GET(_Sequence);
    CK_PROPERTY_GET(_Name);
};

// ----

class USkeleton;

UCLASS(BlueprintType)
class CKISKMRENDERER_API UCk_IskmAnimCollection_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IskmAnimCollection_Data);

protected:
#if WITH_EDITOR
    auto
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;

    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    // ---- core animation data ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeleton> _Skeleton;

    // Mesh used as the source for bone-index buffers and the reference pose. NOT the
    // visible character mesh — that's `_Submeshes` on the Renderer PDA.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeletalMesh> _DefaultMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true, TitleProperty = "{_Name}"))
    TArray<FCk_IskmAnimCollection_SequenceDef> _Sequences;

    // Plan-2: frames-per-second the bake samples each sequence at. Higher = smoother + more VRAM.
    // Skelot uses 10..60 per sequence; we use one rate for the whole collection (matches Skelot's common case).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true, UIMin = 1, ClampMin = 1, UIMax = 120, ClampMax = 120))
    int32 _SampleFrequency = 30;

    // ---- Plan-2 reservations (declared now so we don't migrate .uassets later) ----

    // Bones whose CPU transforms are cached per-frame for socket attaching / cheap line
    // trace. Plan-1 leaves this empty; Plan-2 reads it during the bake.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    TSet<FName> _BonesToCache;

    // Animation curves sampled into VRAM, accessible from materials via GetCurveValue().
    // Plan-2 only — declared now to lock asset shape.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _CurvesToCache;

    // GPU pose-buffer pool sizes. Plan-2 only.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _MaxTransitionPose = 2000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _MaxDynamicPose = 0;

    // Anim-bake flags consumed by Plan-2's GPU bake pass. Plan-1 ignores them.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _bExtractRootMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _bHighPrecision = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _bDisableRetargeting = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _bDontGenerateBounds = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _bCachePhysicsAssetBones = false;

    // ---- Plan-2 transient bake (not reflected, not serialized; rebuilt on demand) ----
    TPimplPtr<FCk_Iskm_BakedPose> _BakedPose;

public:
    CK_PROPERTY_GET(_Skeleton);
    CK_PROPERTY_GET(_DefaultMesh);
    CK_PROPERTY_GET(_Sequences);
    CK_PROPERTY_GET(_SampleFrequency);
    // Plan-2 reservation accessors (read-only; nothing in Plan-1 reads these)
    CK_PROPERTY_GET(_BonesToCache);
    CK_PROPERTY_GET(_CurvesToCache);
    CK_PROPERTY_GET(_MaxTransitionPose);
    CK_PROPERTY_GET(_MaxDynamicPose);
    CK_PROPERTY_GET(_bExtractRootMotion);
    CK_PROPERTY_GET(_bHighPrecision);
    CK_PROPERTY_GET(_bDisableRetargeting);
    CK_PROPERTY_GET(_bDontGenerateBounds);
    CK_PROPERTY_GET(_bCachePhysicsAssetBones);

public:
    auto
    Find_SequenceIndex_ByAsset(const UAnimSequenceBase* InAsset) const -> int32;

    // ---- Plan-2 CPU bake ----

    // Builds (or rebuilds) the CPU bone-matrix bake from the skeleton + default mesh + sequences.
    // CPU-only and headless-safe (touches no RHI). Returns false if the asset is not bakeable
    // (missing skeleton/mesh, no skinned bones). On success Get_IsBaked() becomes true.
    auto
    Build_BakedPoseData() -> bool;

    // The baked pose data, or nullptr if Build_BakedPoseData() has not succeeded.
    auto
    Get_BakedPose() const -> const FCk_Iskm_BakedPose*;

    auto
    Get_IsBaked() const -> bool;
};
