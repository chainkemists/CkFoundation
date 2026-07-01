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
    };

    // Bind the AnimCollection (provides the baked SRV/UB + per-mesh render data) and the visible mesh to draw.
    void Setup(UCk_IskmAnimCollection_Data* InCollection, USkeletalMesh* InMesh);

    // Replace the instance set; recomputes bounds + per-component frame, then recreates the proxy.
    void Set_Instances(const TArray<FInstance>& InInstances);
    const TArray<FInstance>& Get_Instances() const { return _Instances; }

    UCk_IskmAnimCollection_Data* Get_AnimCollection() const { return _AnimCollection; }
    USkeletalMesh* Get_Mesh() const { return _Mesh; }

    //~ UPrimitiveComponent
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;
    virtual int32 GetNumMaterials() const override;
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
    UPROPERTY(Transient) TObjectPtr<UCk_IskmAnimCollection_Data> _AnimCollection;
    UPROPERTY(Transient) TObjectPtr<USkeletalMesh> _Mesh;

    TArray<FInstance> _Instances;
    FBox _LocalBounds = FBox(ForceInit);

    void Recompute_LocalBounds();
    // Phase 1-2: push instances[0]'s frame into CustomPrimitiveData[0/1] (per-component, raw int bits as float).
    void Refresh_PerComponentFrame();
};
