#include "CkIskm_BatchedClusterComponent.h"

#include "CkIskm_BatchedClusterProxy.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_BakedPose.h"

#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "RHIDefinitions.h"
#include "RenderUtils.h"
#include "RenderingThread.h"

UCk_Iskm_BatchedClusterComponent::UCk_Iskm_BatchedClusterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    bSelectable = false;
    CastShadow = true;
    bUseAsOccluder = false;
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Setup(UCk_IskmAnimCollection_Data* InCollection, USkeletalMesh* InMesh)
    -> void
{
    _AnimCollection = InCollection;
    _Mesh = InMesh;
    Recompute_LocalBounds();
    UpdateBounds();
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Set_Instances(const TArray<FInstance>& InInstances)
    -> void
{
    _Instances = InInstances;
    for (FInstance& Inst : _Instances)
    { Inst.PrevPushedTransform = Inst.Transform; } // no pre-history — first frame renders with zero velocity
    Recompute_LocalBounds();
    UpdateBounds();
    MarkRenderStateDirty();
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Push_LiveInstances(TArray<FInstance>&& InInstances)
    -> void
{
    _Instances = MoveTemp(InInstances);
    MarkRenderDynamicDataDirty();
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Set_ManagedExternally(bool InManaged)
    -> void
{
    _ManagedExternally = InManaged;
    SetComponentTickEnabled(NOT InManaged);
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Set_FixedLocalBounds(const FBox& InLocalBounds)
    -> void
{
    _FixedLocalBounds = InLocalBounds;
    Recompute_LocalBounds();
    UpdateBounds();
}

auto
    UCk_Iskm_BatchedClusterComponent::
    CreateSceneProxy()
    -> FPrimitiveSceneProxy*
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

auto
    UCk_Iskm_BatchedClusterComponent::
    Set_OverrideMaterial(UMaterialInterface* InMaterial)
    -> void
{
    if (_OverrideMaterial == InMaterial)
    { return; }
    _OverrideMaterial = InMaterial;
    MarkRenderStateDirty(); // proxy caches materials at construction — recreate it
}

auto
    UCk_Iskm_BatchedClusterComponent::
    GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool InGetDebugMaterials) const
    -> void
{
    if (_Mesh == nullptr)
    { return; }
    if (_OverrideMaterial != nullptr)
    {
        OutMaterials.Add(_OverrideMaterial);
        return;
    }
    for (const FSkeletalMaterial& M : _Mesh->GetMaterials())
    {
        OutMaterials.Add(M.MaterialInterface);
    }
}

auto
    UCk_Iskm_BatchedClusterComponent::
    GetNumMaterials() const
    -> int32
{
    return _Mesh != nullptr ? _Mesh->GetNumMaterials() : 0;
}

auto
    UCk_Iskm_BatchedClusterComponent::
    GetMaterial(int32 ElementIndex) const
    -> UMaterialInterface*
{
    if (_Mesh == nullptr)
    { return nullptr; }
    if (_OverrideMaterial != nullptr)
    { return _OverrideMaterial; }
    const TArray<FSkeletalMaterial>& Mats = _Mesh->GetMaterials();
    return Mats.IsValidIndex(ElementIndex) ? Mats[ElementIndex].MaterialInterface : nullptr;
}

auto
    UCk_Iskm_BatchedClusterComponent::
    CalcBounds(const FTransform& LocalToWorld) const
    -> FBoxSphereBounds
{
    if (_LocalBounds.IsValid == 0)
    {
        return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1.0f), 1.0f);
    }
    return FBoxSphereBounds(_LocalBounds).TransformBy(LocalToWorld);
}

auto
    UCk_Iskm_BatchedClusterComponent::
    Recompute_LocalBounds()
    -> void
{
    // Fixed conservative bounds (tile extent + pad) trump the per-instance union — movement never recomputes.
    if (_FixedLocalBounds.IsValid != 0)
    {
        _LocalBounds = _FixedLocalBounds;
        return;
    }

    _LocalBounds = FBox(ForceInit);
    if (_Mesh == nullptr)
    { return; }

    // Animated bounds (bone union + skin pad) once baked — the raw mesh box clips animated silhouettes.
    const FBox MeshBox = (_AnimCollection != nullptr) ? _AnimCollection->Get_AnimatedMeshBounds() : _Mesh->GetBounds().GetBox();
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

auto
    UCk_Iskm_BatchedClusterComponent::
    TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
    -> void
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Rendering is client-local; a dedicated server has no proxy to feed — skip the animation advance entirely.
    if (GetNetMode() == NM_DedicatedServer)
    { return; }
    if (_AnimCollection == nullptr || _Instances.Num() == 0)
    { return; }
    const FCk_Iskm_BakedPose* Baked = _AnimCollection->Get_BakedPose();
    if (Baked == nullptr)
    { return; }

    bool AnyChanged = false;
    for (FInstance& Inst : _Instances)
    {
        if (Inst.Rate == 0.0f)
        { continue; }
        Inst.Time += DeltaTime * Inst.Rate;
        const int32 NewFrame = Baked->Get_LoopedFrameAtTime(Inst.SequenceIndex, Inst.Time);
        Inst.PrevFrame = Inst.CurFrame;
        Inst.CurFrame = NewFrame;
        if (NewFrame != Inst.PrevFrame)
        { AnyChanged = true; }
    }

    if (AnyChanged)
    { MarkRenderDynamicDataDirty(); }
}

auto
    UCk_Iskm_BatchedClusterComponent::
    SendRenderDynamicData_Concurrent()
    -> void
{
    Super::SendRenderDynamicData_Concurrent();

    if (SceneProxy == nullptr)
    { return; }

    const int32 N = _Instances.Num();
    FCk_Iskm_CompDynData* DynData = new FCk_Iskm_CompDynData();
    DynData->Transforms.Reserve(N);
    DynData->PrevTransforms.Reserve(N);
    DynData->NumCustomDataFloats = NumCustomDataFloats; // [Cur, Pre, UserData[0..13]] per instance (% 4 == 0)
    DynData->CustomData.SetNumZeroed(N * NumCustomDataFloats);

    for (int32 i = 0; i < N; ++i)
    {
        FInstance& Inst = _Instances[i];
        DynData->Transforms.Add(FRenderTransform(Inst.Transform.ToMatrixWithScale()));
        // Real motion vectors: previous = the transform we pushed LAST frame, then roll history forward.
        DynData->PrevTransforms.Add(FRenderTransform(Inst.PrevPushedTransform.ToMatrixWithScale()));
        Inst.PrevPushedTransform = Inst.Transform;

        float CurBits = 0.0f;
        float PreBits = 0.0f;
        FMemory::Memcpy(&CurBits, &Inst.CurFrame, sizeof(float));
        FMemory::Memcpy(&PreBits, &Inst.PrevFrame, sizeof(float));
        float* const Dst = &DynData->CustomData[i * NumCustomDataFloats];
        Dst[0] = CurBits;
        Dst[1] = PreBits;
        FMemory::Memcpy(Dst + 2, Inst.UserData, sizeof(Inst.UserData));
    }

    // Per-INSTANCE local bound: one animated-pose box around each instance transform (shared, engine clamps 1-or-N).
    const FBox MeshBox = (_AnimCollection != nullptr) ? _AnimCollection->Get_AnimatedMeshBounds()
                       : (_Mesh != nullptr) ? _Mesh->GetBounds().GetBox() : FBox(FVector(-1.0), FVector(1.0));
    DynData->LocalBounds = FRenderBounds(MeshBox);
    // PRIMITIVE bounds: must cover the WHOLE instance spread. Using the single mesh box here (the old code)
    // collapsed the primitive's scene bounds to one mesh at the component origin every animated frame —
    // FUpdateTransformCommand then replaced the registration bounds, so everything away from the tile centre
    // was wrongly frustum/occlusion culled ("flickers unless looking at the spawn point").
    DynData->LocalBoundsSphere = (_LocalBounds.IsValid != 0) ? FBoxSphereBounds(_LocalBounds) : FBoxSphereBounds(MeshBox);
    const FTransform CompXf = GetComponentTransform();
    DynData->LocalToWorld = CompXf.ToMatrixWithScale();
    DynData->PrevLocalToWorld = DynData->LocalToWorld;
    DynData->WorldBounds = DynData->LocalBoundsSphere.TransformBy(CompXf);

    FCk_Iskm_BatchedClusterProxy* Proxy = static_cast<FCk_Iskm_BatchedClusterProxy*>(SceneProxy);
    ENQUEUE_RENDER_COMMAND(CkIskm_UpdateBatchedCluster)(
        [Proxy, DynData](FRHICommandListImmediate&)
        {
            Proxy->UpdateInstanceBuffer(DynData);
        });
}
