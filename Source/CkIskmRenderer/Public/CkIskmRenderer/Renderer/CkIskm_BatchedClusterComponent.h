#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

#include "CkIskm_BatchedClusterComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// CkIskmRenderer Plan-2 — one spatial cluster of batched skeletal instances; lives on the per-world manager actor.

class UCk_IskmAnimCollection_Data;
class USkeletalMesh;

UCLASS(ClassGroup = (Ck), NotBlueprintable, meta = (BlueprintSpawnableComponent))
class CKISKMRENDERER_API UCk_Iskm_BatchedClusterComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UCk_Iskm_BatchedClusterComponent();

    // GPU per-instance custom-data layout: [0]=CurFrame bits, [1]=PrevFrame bits,
    // [2..15]=FInstance::UserData[0..13] (the material PerInstanceCustomData DataIndex space,
    // whose game-side meaning the game owns).
    static constexpr int32 NumCustomDataFloats = 16; // must stay % 4 == 0
    static constexpr int32 NumUserDataFloats   = 14; // NumCustomDataFloats - 2 frame floats

    struct FInstance
    {
        FTransform Transform = FTransform::Identity; // relative to the component
        int32 CurFrame = 0;
        int32 PrevFrame = 0;
        float Time = 0.0f;
        float Rate = 0.0f;      // 0 = static (holds CurFrame); >0 = animate at this multiplier
        int32 SequenceIndex = 0;
        // Last transform pushed to the render thread — the motion-vector source. Without it, moving instances
        // upload Prev==Current (zero velocity) and ghost under TAA. Seeded to Transform on Set_Instances.
        FTransform PrevPushedTransform = FTransform::Identity;
        // Shader-visible custom-data floats [2..15] — per-instance material variety.
        float UserData[NumUserDataFloats] = {};
    };

    void Setup(UCk_IskmAnimCollection_Data* InCollection, USkeletalMesh* InMesh);

    // Replace the instance set; recomputes bounds + per-component frame, then recreates the proxy.
    void Set_Instances(const TArray<FInstance>& InInstances);
    const TArray<FInstance>& Get_Instances() const { return _Instances; }

    // Light per-frame path: replace instance data without recreating the proxy or recomputing bounds.
    // The instance COUNT must match the last Set_Instances — count changes must go through Set_Instances.
    void Push_LiveInstances(TArray<FInstance>&& InInstances);

    // Managed mode: an external owner advances animation and pushes per-frame data — disables self-tick.
    void Set_ManagedExternally(bool InManaged);

    // Fixed conservative local bounds (e.g. tile extent + mesh pad) — movement inside them never recomputes.
    void Set_FixedLocalBounds(const FBox& InLocalBounds);

    // One material applied to EVERY slot of every instance. Recreates the proxy; null restores the
    // mesh's own materials.
    void Set_OverrideMaterial(UMaterialInterface* InMaterial);

    // Per-SLOT override materials: element index = mesh material slot. Set_OverrideMaterial above WINS
    // over these; null/absent entries fall back to mesh defaults. Recreates the proxy.
    void Set_SlotOverrideMaterials(const TArray<UMaterialInterface*>& InMaterials);
    UMaterialInterface* Get_SlotOverrideMaterial(int32 InSlotIndex) const;

    UMaterialInterface* Get_OverrideMaterial() const { return _OverrideMaterial; }
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
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> _OverrideMaterial;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> _SlotOverrideMaterials;

    TArray<FInstance> _Instances;
    FBox _LocalBounds = FBox(ForceInit);
    FBox _FixedLocalBounds = FBox(ForceInit); // when valid, overrides the per-instance union
    bool _ManagedExternally = false;

    void Recompute_LocalBounds();
};
