#include "CkIskm_BatchedClusterComponent.h"

#include "CkIskm_BatchedClusterProxy.h"
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
    for (FInstance& Inst : _Instances)
    { Inst.PrevPushedTransform = Inst.Transform; } // no pre-history — first frame renders with zero velocity
    Recompute_LocalBounds();
    UpdateBounds();
    MarkRenderStateDirty();
}

void
    UCk_Iskm_BatchedClusterComponent::
    Push_LiveInstances(TArray<FInstance>&& InInstances)
{
    _Instances = MoveTemp(InInstances);
    MarkRenderDynamicDataDirty();
}

void
    UCk_Iskm_BatchedClusterComponent::
    Set_ManagedExternally(bool bInManaged)
{
    _ManagedExternally = bInManaged;
    SetComponentTickEnabled(!bInManaged);
}

void
    UCk_Iskm_BatchedClusterComponent::
    Set_FixedLocalBounds(const FBox& InLocalBounds)
{
    _FixedLocalBounds = InLocalBounds;
    Recompute_LocalBounds();
    UpdateBounds();
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

void
    UCk_Iskm_BatchedClusterComponent::
    TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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

    bool bAnyChanged = false;
    for (FInstance& Inst : _Instances)
    {
        if (Inst.Rate == 0.0f)
        { continue; }
        Inst.Time += DeltaTime * Inst.Rate;
        const int32 NewFrame = Baked->Get_LoopedFrameAtTime(Inst.SequenceIndex, Inst.Time);
        Inst.PrevFrame = Inst.CurFrame;
        Inst.CurFrame = NewFrame;
        if (NewFrame != Inst.PrevFrame)
        { bAnyChanged = true; }
    }

    if (bAnyChanged)
    { MarkRenderDynamicDataDirty(); }
}

void
    UCk_Iskm_BatchedClusterComponent::
    SendRenderDynamicData_Concurrent()
{
    Super::SendRenderDynamicData_Concurrent();

    if (SceneProxy == nullptr)
    { return; }

    const int32 N = _Instances.Num();
    FCk_Iskm_CompDynData* DynData = new FCk_Iskm_CompDynData();
    DynData->Transforms.Reserve(N);
    DynData->PrevTransforms.Reserve(N);
    DynData->NumCustomDataFloats = 4; // [Cur, Pre, UserA, UserB] per instance (must be % 4 == 0)
    DynData->CustomData.SetNumZeroed(N * 4);

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
        DynData->CustomData[i * 4 + 0] = CurBits;
        DynData->CustomData[i * 4 + 1] = PreBits;
        DynData->CustomData[i * 4 + 2] = Inst.CustomDataA;
        DynData->CustomData[i * 4 + 3] = Inst.CustomDataB;
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
