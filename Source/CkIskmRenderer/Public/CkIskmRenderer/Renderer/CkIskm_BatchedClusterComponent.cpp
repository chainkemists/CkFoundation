#include "CkIskm_BatchedClusterComponent.h"

#include "CkIskm_BatchedClusterProxy.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "RHIDefinitions.h"
#include "RenderUtils.h"

UCk_Iskm_BatchedClusterComponent::UCk_Iskm_BatchedClusterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    bSelectable = false;
    CastShadow = true;
    bUseAsOccluder = false;
}

void
    UCk_Iskm_BatchedClusterComponent::
    Setup(UCk_IskmAnimCollection_Data* InCollection, USkeletalMesh* InMesh)
{
    _AnimCollection = InCollection;
    _Mesh = InMesh;
    Recompute_LocalBounds();
    UpdateBounds();
}

void
    UCk_Iskm_BatchedClusterComponent::
    Set_Instances(const TArray<FInstance>& InInstances)
{
    _Instances = InInstances;
    Recompute_LocalBounds();
    Refresh_PerComponentFrame();
    UpdateBounds();
    MarkRenderStateDirty();
}

FPrimitiveSceneProxy*
    UCk_Iskm_BatchedClusterComponent::
    CreateSceneProxy()
{
    if (_AnimCollection == nullptr || _Mesh == nullptr || _Instances.Num() == 0)
    { return nullptr; }
    if (_Mesh->GetResourceForRendering() == nullptr)
    { return nullptr; }

    // Idempotent: CPU bake (if needed) + enqueue the GPU SRV/UB + per-mesh render-resource upload.
    _AnimCollection->EnsureRenderResources();
    if (_AnimCollection->Get_IsBaked() == false)
    { return nullptr; }

    return new FCk_Iskm_BatchedClusterProxy(this, GetFName());
}

void
    UCk_Iskm_BatchedClusterComponent::
    GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
    if (_Mesh == nullptr)
    { return; }
    for (const FSkeletalMaterial& M : _Mesh->GetMaterials())
    {
        OutMaterials.Add(M.MaterialInterface);
    }
}

int32
    UCk_Iskm_BatchedClusterComponent::
    GetNumMaterials() const
{
    return _Mesh != nullptr ? _Mesh->GetNumMaterials() : 0;
}

UMaterialInterface*
    UCk_Iskm_BatchedClusterComponent::
    GetMaterial(int32 ElementIndex) const
{
    if (_Mesh == nullptr)
    { return nullptr; }
    const TArray<FSkeletalMaterial>& Mats = _Mesh->GetMaterials();
    return Mats.IsValidIndex(ElementIndex) ? Mats[ElementIndex].MaterialInterface : nullptr;
}

FBoxSphereBounds
    UCk_Iskm_BatchedClusterComponent::
    CalcBounds(const FTransform& LocalToWorld) const
{
    if (_LocalBounds.IsValid == 0)
    {
        return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1.0f), 1.0f);
    }
    return FBoxSphereBounds(_LocalBounds).TransformBy(LocalToWorld);
}

void
    UCk_Iskm_BatchedClusterComponent::
    Recompute_LocalBounds()
{
    _LocalBounds = FBox(ForceInit);
    if (_Mesh == nullptr)
    { return; }

    const FBox MeshBox = _Mesh->GetBounds().GetBox();
    if (_Instances.Num() == 0)
    {
        _LocalBounds = MeshBox;
        return;
    }
    for (const FInstance& Inst : _Instances)
    {
        _LocalBounds += MeshBox.TransformBy(Inst.Transform);
    }
}

void
    UCk_Iskm_BatchedClusterComponent::
    Refresh_PerComponentFrame()
{
    if (_Instances.Num() == 0)
    { return; }

    const int32 Cur = _Instances[0].CurFrame;
    const int32 Pre = _Instances[0].PrevFrame;
    float CurBits = 0.0f;
    float PreBits = 0.0f;
    FMemory::Memcpy(&CurBits, &Cur, sizeof(float));
    FMemory::Memcpy(&PreBits, &Pre, sizeof(float));
    SetCustomPrimitiveDataFloat(0, CurBits);
    SetCustomPrimitiveDataFloat(1, PreBits);
}
