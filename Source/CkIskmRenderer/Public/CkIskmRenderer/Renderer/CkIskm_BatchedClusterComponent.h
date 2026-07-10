#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

#include "CkIskm_BatchedClusterComponent.generated.h"

// ====================================================================================================================
//  CkIskmRenderer Plan-2 — cluster component owning a set of batched skeletal instances + creating the cluster proxy.
//  Analogue of Skelot's USkelotClusterComponent. One per spatial cluster; lives on the per-world manager actor.
//
//  Phase 1-2: instances share one baked frame, carried in per-component CustomPrimitiveData[0] (no material flag).
//  Phase 3+: per-instance frames via the proxy's GPUScene per-instance custom data.
// ====================================================================================================================

class UCk_IskmAnimCollection_Data;
class USkeletalMesh;

UCLASS(ClassGroup = (Ck), NotBlueprintable, meta = (BlueprintSpawnableComponent))
class CKISKMRENDERER_API UCk_Iskm_BatchedClusterComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UCk_Iskm_BatchedClusterComponent();

    struct FInstance
    {
        FTransform Transform = FTransform::Identity; // relative to the component
        int32 CurFrame = 0;
        int32 PrevFrame = 0;
        // Per-instance independent animation (Phase 3): advanced each tick to a looped baked frame.
        float Time = 0.0f;
        float Rate = 0.0f;      // 0 = static (holds CurFrame); >0 = animate at this multiplier
        int32 SequenceIndex = 0;
        // Last transform pushed to the render thread — the motion-vector source. Without it, moving instances
        // upload Prev==Current (zero velocity) and ghost under TAA. Seeded to Transform on Set_Instances.
        FTransform PrevPushedTransform = FTransform::Identity;
        // Per-instance material custom data, surfaced to the shader as instance custom-data floats [2] and [3]
        // ([0]/[1] carry the animation frame indices). Drives per-instance material variety (tint etc.).
        float CustomDataA = 0.0f;
        float CustomDataB = 0.0f;
    };

    // Bind the AnimCollection (provides the baked SRV/UB + per-mesh render data) and the visible mesh to draw.
    void Setup(UCk_IskmAnimCollection_Data* InCollection, USkeletalMesh* InMesh);

    // Replace the instance set; recomputes bounds + per-component frame, then recreates the proxy.
    void Set_Instances(const TArray<FInstance>& InInstances);
    const TArray<FInstance>& Get_Instances() const { return _Instances; }

    // Light per-frame path for an externally-managed component: replace instance data WITHOUT recreating the
    // proxy or recomputing bounds. Caller must keep the instance COUNT identical to the last Set_Instances
    // (count changes must go through Set_Instances) — enforced with an ensure at the call site in the manager.
    void Push_LiveInstances(TArray<FInstance>&& InInstances);

    // Managed mode: an external owner (the crowd manager) advances animation and pushes per-frame data;
    // disable this component's self-tick.
    void Set_ManagedExternally(bool InManaged);

    // Fixed conservative local bounds (e.g. tile extent + mesh pad) — movement inside them never recomputes.
    void Set_FixedLocalBounds(const FBox& InLocalBounds);

    UCk_IskmAnimCollection_Data* Get_AnimCollection() const { return _AnimCollection; }
    USkeletalMesh* Get_Mesh() const { return _Mesh; }
    const FBox& Get_LocalBounds() const { return _LocalBounds; }

    //~ UActorComponent
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //~ UPrimitiveComponent
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual void SendRenderDynamicData_Concurrent() override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool InGetDebugMaterials) const override;
    virtual int32 GetNumMaterials() const override;
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
    UPROPERTY(Transient) TObjectPtr<UCk_IskmAnimCollection_Data> _AnimCollection;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> _Mesh;

    TArray<FInstance> _Instances;
    FBox _LocalBounds = FBox(ForceInit);
    FBox _FixedLocalBounds = FBox(ForceInit); // when valid, overrides the per-instance union
    bool _ManagedExternally = false;

    void Recompute_LocalBounds();
};
