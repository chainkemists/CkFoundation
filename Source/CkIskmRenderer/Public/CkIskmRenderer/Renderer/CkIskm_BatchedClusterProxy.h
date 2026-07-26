#pragma once

#include "CoreMinimal.h"

#include "CkCore/Macros/CkMacros.h"

#include "PrimitiveSceneProxy.h"
#include "InstanceDataSceneProxy.h"

// --------------------------------------------------------------------------------------------------------------------
// CkIskmRenderer Plan-2 — cluster scene proxy for batched GPU-skinned skeletal instancing (GPUScene desktop only).
// Every instance is a GPUScene instance under this one FPrimitiveSceneProxy; its animation frame index travels via
// per-instance custom data, which the vertex factory reads to skin against the matching baked frame.

class UCk_Iskm_BatchedClusterComponent;
class UCk_IskmAnimCollection_Data;
class USkeletalMesh;
class FCk_Iskm_BatchedClusterProxy;

// Contiguous instance range [StartOffset, EndOffset] (inclusive) — laid out to alias the engine's instance-run uint32 pairs.
struct FCk_Iskm_InstanceRun
{
    uint32 StartOffset = 0;
    uint32 EndOffset = 0;

    FORCEINLINE int32 Num() const { return static_cast<int32>(EndOffset) - static_cast<int32>(StartOffset) + 1; }
};

// Per-frame instance payload pushed game-thread -> render-thread. Element types are exact (FRenderTransform / FRenderBounds).
struct FCk_Iskm_CompDynData
{
    TArray<FRenderTransform> Transforms;          // InstanceToPrimitiveRelative
    TArray<FRenderTransform> PrevTransforms;       // PrevInstanceToPrimitiveRelative (velocity)
    TArray<float>            CustomData;            // [Cur(int bits), Pre(int bits), user...] per instance, padded to %4
    int32                    NumCustomDataFloats = 0;
    TArray<FRenderBounds>    InstanceLocalBounds;   // per-instance or a single shared bound
    FRenderBounds            LocalBounds;                                  // for the ISDB (FRenderBounds)
    FBoxSphereBounds         WorldBounds = FBoxSphereBounds(ForceInit);    // for the scene-update commands (FBoxSphereBounds)
    FBoxSphereBounds         LocalBoundsSphere = FBoxSphereBounds(ForceInit);
    FMatrix                  LocalToWorld = FMatrix::Identity;
    FMatrix                  PrevLocalToWorld = FMatrix::Identity;
};

struct FCk_Iskm_ClusterInstanceData : public FInstanceSceneDataBuffers
{
    FCk_Iskm_ClusterInstanceData();
    void Init(bool InWithPerInstanceLocalBounds);
};

// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERER_API FCk_Iskm_BatchedClusterProxy : public FPrimitiveSceneProxy
{
public:
    FCk_Iskm_BatchedClusterProxy(UCk_Iskm_BatchedClusterComponent* InComponent, FName InResourceName);
    virtual ~FCk_Iskm_BatchedClusterProxy() override;

    // FPrimitiveSceneProxy
    virtual SIZE_T GetTypeHash() const override;
    virtual uint32 GetMemoryFootprint() const override { return sizeof(*this) + GetAllocatedSize(); }
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
    virtual bool CanBeOccluded() const override { return NOT MaterialRelevance.bDisableDepthTest; }
    // Per-instance GPU occlusion (HZB) culling instead of one occlusion unit per cluster. The single shared
    // InstanceLocalBounds is valid here — GetInstanceLocalBounds clamps 1-or-N.
    virtual bool AllowInstanceCullingOcclusionQueries() const override { return true; }
    virtual void CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
    virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
    virtual float GetGpuLodInstanceRadius() const override { return GPULODRadius; }

    // Render-thread: replace the instance payload + notify the scene (post-registration updates).
    void UpdateInstanceBuffer(FCk_Iskm_CompDynData* InDynData);

private:
    // Writes the currently-stored DynData into the GPUScene instance buffer (render thread).
    void WriteInstanceBuffer();
    void FillMeshBatch(FMeshBatch& OutMesh, int32 InLODIndex, int32 InSectionIndex, const class FSkeletalMeshLODRenderData& InLODData) const;

    // LIFETIME: raw UObject pointers on a non-UObject proxy, dereferenced on the render thread. GC does NOT
    // trace them — safe ONLY because the owning component holds hard UPROPERTY refs to the same objects and
    // the render-state recreate fence brackets the proxy's lifetime (engine's FSkeletalMeshSceneProxy pattern).
    // Smart pointers are NOT render-thread-safe here; if that invariant weakens, snapshot at construction.
    UCk_IskmAnimCollection_Data* AnimCollection = nullptr;
    USkeletalMesh* Mesh = nullptr;
    int32 BaseLOD = 0;
    float GPULODRadius = 0.0f;
    bool WithPerInstanceLocalBound = false;

    FMaterialRelevance MaterialRelevance;
    TArray<UMaterialInterface*> Materials;

    FCk_Iskm_ClusterInstanceData InstanceBuffer;
    TUniquePtr<FCk_Iskm_CompDynData> DynData;

    // STABLE storage: DrawStaticElements caches a raw pointer to this into the static FMeshBatchElement, which
    // InstanceCullingContext dereferences EVERY FRAME. It MUST therefore outlive the per-frame DynData swaps in
    // UpdateInstanceBuffer — backing RunArray with DynData dangles from frame 1 on (garbage ranges -> flicker).
    // Seeded once in the ctor; N is fixed for the proxy's lifetime (Set_Instances recreates the proxy).
    TArray<FCk_Iskm_InstanceRun> StableInstanceRuns;
};
