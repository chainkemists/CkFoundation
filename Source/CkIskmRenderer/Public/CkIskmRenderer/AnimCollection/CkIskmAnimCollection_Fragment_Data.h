#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "Templates/PimplPtr.h"
#include "RenderCommandFence.h"

#include "CkIskmAnimCollection_Fragment_Data.generated.h"

// Plan-2 transient CPU bone-matrix bake (see CkIskmAnimCollection_BakedPose.h). Forward-declared here so the
// widely-included asset header stays light; the full types are only pulled into the .cpp.
struct FCk_Iskm_BakedPose;
// Plan-2 render-thread resources (the baked SRV, its uniform buffer, the default mesh's vertex factories).
struct FCk_Iskm_BatchedRenderData;
class FCk_Iskm_BatchedMeshData;

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

    // Sockets (or bare bone names) whose component-space transforms are baked per-frame into the
    // pose data (inc-4 far cosmetics) — queryable per member via
    // ACk_Iskm_BatchedCrowd_Actor::TryGet_MemberSocketTransform. Script-authored collections
    // configure this (BB adds head_attach), so it must stay BlueprintReadWrite like _Sequences
    // (_BonesToCache above is BlueprintReadOnly and NOT script-assignable — do not reuse it).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _BakedSockets;

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
    bool _ExtractRootMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _HighPrecision = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _DisableRetargeting = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _DontGenerateBounds = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plan-2 Reservations",
              meta = (AllowPrivateAccess = true))
    bool _CachePhysicsAssetBones = false;

    // ---- Plan-2 transient bake (not reflected, not serialized; rebuilt on demand) ----
    TPimplPtr<FCk_Iskm_BakedPose> _BakedPose;
    // ---- Plan-2 transient render resources (built from the bake; render-thread lifetime) ----
    TPimplPtr<FCk_Iskm_BatchedRenderData> _RenderData;
    FRenderCommandFence _ReleaseResourcesFence;

public:
    // _Skeleton with a fallback to the DefaultMesh's own skeleton. Script-authored collections
    // (e.g. BusterBlock's BB_Npc_Iskm_Assets.as) cannot assign _Skeleton — AS only reflects the
    // const USkeletalMesh::GetSkeleton() overload — and Plan-1 never needed it. Everything on the
    // batched (Plan-2) path must resolve the skeleton through this, never read _Skeleton raw.
    auto
    Get_EffectiveSkeleton() const -> USkeleton*;

    CK_PROPERTY_GET(_Skeleton);
    CK_PROPERTY_GET(_DefaultMesh);
    CK_PROPERTY_GET(_Sequences);
    CK_PROPERTY_GET(_SampleFrequency);
    CK_PROPERTY_GET(_BakedSockets);
    // Plan-2 reservation accessors (read-only; nothing in Plan-1 reads these)
    CK_PROPERTY_GET(_BonesToCache);
    CK_PROPERTY_GET(_CurvesToCache);
    CK_PROPERTY_GET(_MaxTransitionPose);
    CK_PROPERTY_GET(_MaxDynamicPose);
    CK_PROPERTY_GET(_ExtractRootMotion);
    CK_PROPERTY_GET(_HighPrecision);
    CK_PROPERTY_GET(_DisableRetargeting);
    CK_PROPERTY_GET(_DontGenerateBounds);
    CK_PROPERTY_GET(_CachePhysicsAssetBones);

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

    // Conservative culling bounds covering every baked pose (bone union + ref-pose skin pad, never smaller
    // than the mesh box). Falls back to the mesh box pre-bake. Use this — not the raw mesh bounds — wherever
    // batched instances are culled, or animated silhouettes clip at bound edges.
    auto
    Get_AnimatedMeshBounds() const -> FBox;

    // ---- Plan-2 GPU render resources ----

    // Idempotent: ensures the CPU bake exists, then builds + uploads the GPU SRV / uniform buffer / per-mesh
    // vertex factories (render thread). No-op when the app cannot render (headless / -nullrhi).
    auto
    EnsureRenderResources() -> void;

    // The default mesh's render data (bone-index buffers + vertex factories), or null until EnsureRenderResources.
    auto
    Get_DefaultMeshData() const -> const FCk_Iskm_BatchedMeshData*;

    //~ UObject — release render resources on the render thread before destruction.
    virtual auto BeginDestroy() -> void override;
    virtual auto IsReadyForFinishDestroy() -> bool override;

private:
    // Called from BeginDestroy only. Does NOT reset _RenderData — the _ReleaseResourcesFence keeps the resource
    // objects alive until their render-thread ReleaseRHI completes (UObject destruction is gated on the fence).
    auto
    ReleaseRenderResources() -> void;
};
