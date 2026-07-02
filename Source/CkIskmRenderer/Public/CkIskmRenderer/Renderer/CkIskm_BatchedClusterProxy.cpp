#include "CkIskm_BatchedClusterProxy.h"

#include "CkIskm_BatchedClusterComponent.h"
#include "CkIskmRendererVF/CkIskm_BatchedRenderResources.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "SceneManagement.h"
#include "SceneInterface.h"

// Plan-2 Phase 3: FScene::PrimitiveUpdates + the FUpdate*Command structs are in PRIVATE renderer headers.
// The #define private public hack exposes FScene internals (Skelot pattern; version-fragile — re-verify on bumps).
#ifndef private
#define private public
#endif
#include "ScenePrivate.h"
#undef private
#include "ScenePrimitiveUpdates.h"

// ====================================================================================================================
//  FCk_Iskm_ClusterInstanceData
// ====================================================================================================================
FCk_Iskm_ClusterInstanceData::FCk_Iskm_ClusterInstanceData()
    : FInstanceSceneDataBuffers()
{
}

void
    FCk_Iskm_ClusterInstanceData::
    Init(bool bWithPerInstanceLocalBounds)
{
    // No buffer-level init required on this fork — per-instance data flags are written through the FWriteView
    // (see FCk_Iskm_BatchedClusterProxy::WriteInstanceBuffer).
}

// ====================================================================================================================
//  Initial dynamic-data build (game thread).
// ====================================================================================================================
namespace ck_iskm_proxy
{
    static void BuildDynData(const UCk_Iskm_BatchedClusterComponent* InComponent, USkeletalMesh* InMesh, FCk_Iskm_CompDynData& Out)
    {
        const TArray<UCk_Iskm_BatchedClusterComponent::FInstance>& Instances = InComponent->Get_Instances();
        const int32 N = Instances.Num();

        // Seed PER-INSTANCE custom data from the start (not per-component CustomPrimitiveData[0]) so a static cluster
        // with distinct per-instance frames renders correctly, and there's no per-component->per-instance pop on the
        // first animated frame. [asfloat(Cur), asfloat(Pre), 0, 0] per instance, matching SendRenderDynamicData.
        Out.Transforms.Reserve(N);
        Out.PrevTransforms.Reserve(N);
        Out.NumCustomDataFloats = 4;
        Out.CustomData.SetNumZeroed(N * 4);
        for (int32 i = 0; i < N; ++i)
        {
            const UCk_Iskm_BatchedClusterComponent::FInstance& Inst = Instances[i];
            const FMatrix M = Inst.Transform.ToMatrixWithScale();
            Out.Transforms.Add(FRenderTransform(M));
            Out.PrevTransforms.Add(FRenderTransform(M));

            float CurBits = 0.0f;
            float PreBits = 0.0f;
            FMemory::Memcpy(&CurBits, &Inst.CurFrame, sizeof(float));
            FMemory::Memcpy(&PreBits, &Inst.PrevFrame, sizeof(float));
            Out.CustomData[i * 4 + 0] = CurBits;
            Out.CustomData[i * 4 + 1] = PreBits;
            Out.CustomData[i * 4 + 2] = Inst.CustomDataA;
            Out.CustomData[i * 4 + 3] = Inst.CustomDataB;
        }

        // Per-INSTANCE local bound = one mesh box (engine applies it per instance transform). PRIMITIVE bounds must
        // cover the whole instance spread — take the component's unioned local bounds, not a single mesh box.
        const FBox MeshBox = (InMesh != nullptr) ? InMesh->GetBounds().GetBox() : FBox(FVector(-1.0), FVector(1.0));
        Out.LocalBounds = FRenderBounds(MeshBox);

        const FTransform CompXf = InComponent->GetComponentTransform();
        Out.LocalToWorld = CompXf.ToMatrixWithScale();
        Out.PrevLocalToWorld = Out.LocalToWorld;
        const FBox& SpreadBounds = InComponent->Get_LocalBounds();
        Out.LocalBoundsSphere = (SpreadBounds.IsValid != 0) ? FBoxSphereBounds(SpreadBounds) : FBoxSphereBounds(MeshBox);
        Out.WorldBounds = Out.LocalBoundsSphere.TransformBy(CompXf);
    }
}

// ====================================================================================================================
//  FCk_Iskm_BatchedClusterProxy
// ====================================================================================================================
FCk_Iskm_BatchedClusterProxy::FCk_Iskm_BatchedClusterProxy(UCk_Iskm_BatchedClusterComponent* InComponent, FName InResourceName)
    : FPrimitiveSceneProxy(InComponent, InResourceName)
    , InstanceBuffer()
{
    AnimCollection = InComponent->Get_AnimCollection();
    Mesh = InComponent->Get_Mesh();

    bSupportsGPUScene = true;
    bAlwaysHasVelocity = true;
    bHasDeformableMesh = true;
    bHasWorldPositionOffsetVelocity = true;
    bVFRequiresPrimitiveUniformBuffer = false;

    // Materials + relevance.
    Materials.Reset();
    if (Mesh != nullptr)
    {
        const TArray<FSkeletalMaterial>& Mats = Mesh->GetMaterials();
        Materials.Reserve(Mats.Num());
        const ERHIFeatureLevel::Type FeatureLevel = GetScene().GetFeatureLevel();
        const EShaderPlatform ShaderPlatform = GShaderPlatformForFeatureLevel[FeatureLevel];
        for (const FSkeletalMaterial& M : Mats)
        {
            UMaterialInterface* MI = (M.MaterialInterface != nullptr) ? M.MaterialInterface.Get() : UMaterial::GetDefaultMaterial(MD_Surface);
            Materials.Add(MI);
            MaterialRelevance |= MI->GetRelevance_Concurrent(ShaderPlatform);
        }
    }

    if (AnimCollection != nullptr)
    {
        const FCk_Iskm_BatchedMeshData* MeshData = AnimCollection->Get_DefaultMeshData();
        if (MeshData != nullptr)
        { BaseLOD = MeshData->BaseLOD; }
    }

    GPULODRadius = (Mesh != nullptr) ? static_cast<float>(Mesh->GetBounds().SphereRadius) : 1.0f;

    // Initial instance payload (consumed by CreateRenderThreadResources / DrawStaticElements).
    DynData = MakeUnique<FCk_Iskm_CompDynData>();
    ck_iskm_proxy::BuildDynData(InComponent, Mesh, *DynData);

    // Seed the STABLE instance-run storage once — a single contiguous run [0, N-1] over all instances. See the
    // StableInstanceRuns comment in the header: this backs the cached RunArray that the renderer derefs every frame.
    if (const int32 NumInstances = DynData->Transforms.Num(); NumInstances > 0)
    {
        FCk_Iskm_InstanceRun Run;
        Run.StartOffset = 0;
        Run.EndOffset = static_cast<uint32>(NumInstances - 1);
        StableInstanceRuns.Add(Run);
    }

    SetupInstanceSceneDataBuffers(&InstanceBuffer);
    InstanceBuffer.Init(bWithPerInstanceLocalBound);
}

FCk_Iskm_BatchedClusterProxy::~FCk_Iskm_BatchedClusterProxy()
{
}

SIZE_T
    FCk_Iskm_BatchedClusterProxy::
    GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

FPrimitiveViewRelevance
    FCk_Iskm_BatchedClusterProxy::
    GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bStaticRelevance = true;
    Result.bDynamicRelevance = false;
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    Result.bVelocityRelevance = DrawsVelocity();
    MaterialRelevance.SetPrimitiveViewRelevance(Result);
    return Result;
}

void
    FCk_Iskm_BatchedClusterProxy::
    CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
{
    // Populate the GPUScene instance buffer once, before the scene first reads it.
    WriteInstanceBuffer();
}

void
    FCk_Iskm_BatchedClusterProxy::
    WriteInstanceBuffer()
{
    if (DynData.IsValid() == false)
    { return; }

    FInstanceSceneDataBuffers::FAccessTag AT(666);
    FInstanceSceneDataBuffers::FWriteView RW = InstanceBuffer.BeginWriteAccess(AT);

    InstanceBuffer.SetPrimitiveLocalToWorld(GetLocalToWorld(), AT);

    RW.InstanceLocalBounds.Reset(1);
    RW.InstanceLocalBounds.Add(DynData->LocalBounds);

    RW.InstanceToPrimitiveRelative = MoveTemp(DynData->Transforms);
    RW.PrevInstanceToPrimitiveRelative = MoveTemp(DynData->PrevTransforms);
    RW.Flags.bHasPerInstanceDynamicData = 1;

    if (DynData->NumCustomDataFloats > 0)
    {
        RW.NumCustomDataFloats = DynData->NumCustomDataFloats;
        RW.InstanceCustomData = MoveTemp(DynData->CustomData);
        RW.Flags.bHasPerInstanceCustomData = 1;
    }

    InstanceBuffer.EndWriteAccess(AT);
}

void
    FCk_Iskm_BatchedClusterProxy::
    UpdateInstanceBuffer(FCk_Iskm_CompDynData* InDynData)
{
    CK_ENSURE_IF_NOT(IsInRenderingThread(), TEXT("UpdateInstanceBuffer must run on the render thread"))
    {
        // We own InDynData (SendRenderDynamicData_Concurrent new'd it). Free it on the guard path so we don't leak.
        delete InDynData;
        return;
    }

    // Snapshot bounds/transforms before WriteInstanceBuffer MoveTemps the per-instance arrays out of DynData.
    // (Locals suffixed _Snap to avoid shadowing FPrimitiveSceneProxy members LocalToWorld/Scene/LocalBounds.)
    const FBoxSphereBounds WorldBounds_Snap = InDynData->WorldBounds;
    const FBoxSphereBounds LocalBounds_Snap = InDynData->LocalBoundsSphere;
    const FMatrix LocalToWorld_Snap = InDynData->LocalToWorld;
    const FMatrix PrevLocalToWorld_Snap = InDynData->PrevLocalToWorld;

    DynData = TUniquePtr<FCk_Iskm_CompDynData>(InDynData);
    WriteInstanceBuffer();

    FScene* RenderScene = GetScene().GetRenderScene();
    FPrimitiveSceneInfo* PSI = GetPrimitiveSceneInfo();
    if (RenderScene == nullptr || PSI == nullptr)
    { return; }

    // Notify the scene of the new instance transforms + custom data (GPUScene re-uploads the instance buffer).
    RenderScene->PrimitiveUpdates.Enqueue(PSI, FUpdateTransformCommand{
        .WorldBounds = WorldBounds_Snap,
        .LocalBounds = LocalBounds_Snap,
        .LocalToWorld = LocalToWorld_Snap,
        .AttachmentRootPosition = FVector::ZeroVector });
    RenderScene->PrimitiveUpdates.Enqueue(PSI, FUpdateOverridePreviousTransformData(PrevLocalToWorld_Snap));
    RenderScene->PrimitiveUpdates.Enqueue(PSI, FUpdateInstanceCommand{
        .PrimitiveSceneProxy = this,
        .WorldBounds = WorldBounds_Snap,
        .LocalBounds = LocalBounds_Snap });
}

void
    FCk_Iskm_BatchedClusterProxy::
    FillMeshBatch(FMeshBatch& OutMesh, int32 InLODIndex, int32 InSectionIndex, const FSkeletalMeshLODRenderData& InLODData) const
{
    const FSkelMeshRenderSection& Section = InLODData.RenderSections[InSectionIndex];
    FMeshBatchElement& BatchElement = OutMesh.Elements[0];

    OutMesh.SegmentIndex = InSectionIndex;
    OutMesh.MeshIdInPrimitive = InSectionIndex;
    OutMesh.LODIndex = InLODIndex;
    OutMesh.bUseAsOccluder = ShouldUseAsOccluder();
    OutMesh.CastShadow = bCastDynamicShadow && Section.bCastShadow;
    OutMesh.Type = PT_TriangleList;
    OutMesh.bUseForMaterial = true;
    // Flip winding for negative-determinant (mirrored/negative-scale) instances so backface culling stays correct.
    OutMesh.ReverseCulling = IsLocalToWorldDeterminantNegative();

    BatchElement.IndexBuffer = InLODData.MultiSizeIndexContainer.GetIndexBuffer();
    BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
    BatchElement.FirstIndex = Section.BaseIndex;
    BatchElement.NumPrimitives = Section.NumTriangles;
    BatchElement.MinVertexIndex = Section.BaseVertexIndex;
    BatchElement.MaxVertexIndex = InLODData.GetNumVertices() - 1;
    BatchElement.bForceInstanceCulling = true;
    BatchElement.bFetchInstanceCountFromScene = true;
    BatchElement.bPreserveInstanceOrder = false;
    BatchElement.UserData = nullptr;
    BatchElement.VertexFactoryUserData = nullptr;
}

void
    FCk_Iskm_BatchedClusterProxy::
    DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
    if (DynData.IsValid() == false || Mesh == nullptr || AnimCollection == nullptr)
    { return; }
    if (StableInstanceRuns.Num() == 0)
    { return; }

    const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
    if (RenderData == nullptr)
    { return; }
    const FCk_Iskm_BatchedMeshData* MeshData = AnimCollection->Get_DefaultMeshData();
    if (MeshData == nullptr)
    { return; }

    for (int32 LODIndex = BaseLOD; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
    {
        FCk_Iskm_BatchedVertexFactory* VF = MeshData->Get_VertexFactory(LODIndex);
        if (VF == nullptr)
        { continue; }

        const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
        const FSkeletalMeshLODInfo* LODInfo = Mesh->GetLODInfo(LODIndex);
        const float ScreenSize = (LODInfo != nullptr) ? LODInfo->ScreenSize.GetValue() : 1.0f;

        for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); ++SectionIndex)
        {
            const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
            if (Section.NumTriangles == 0)
            { continue; }

            FMeshBatch MeshBatch;
            FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
            FillMeshBatch(MeshBatch, LODIndex, SectionIndex, LODData);
            MeshBatch.VertexFactory = VF;

            const int32 MatIndex = Section.MaterialIndex;
            UMaterialInterface* MI = (Materials.IsValidIndex(MatIndex) && Materials[MatIndex] != nullptr)
                ? Materials[MatIndex] : UMaterial::GetDefaultMaterial(MD_Surface);
            MeshBatch.MaterialRenderProxy = MI->GetRenderProxy();
            MeshBatch.DepthPriorityGroup = GetStaticDepthPriorityGroup();

            BatchElement.MaxScreenSize = ScreenSize;
            BatchElement.MinScreenSize = 0.0f;
            const FSkeletalMeshLODInfo* NextLODInfo = Mesh->GetLODInfo(LODIndex + 1);
            if (NextLODInfo != nullptr)
            { BatchElement.MinScreenSize = NextLODInfo->ScreenSize.GetValue(); }

            BatchElement.bIsInstanceRuns = true;
            BatchElement.NumInstances = StableInstanceRuns.Num();
            BatchElement.InstanceRuns = reinterpret_cast<uint32*>(StableInstanceRuns.GetData());

            PDI->DrawMesh(MeshBatch, ScreenSize);
        }
    }
}
