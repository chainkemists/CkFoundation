#include "CkIskmRendererVF/CkIskm_BatchedRenderResources.h"

#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "RenderUtils.h"
#include "GlobalRenderResources.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FCk_Iskm_AnimCollectionUniformParams, "CkIskmAC");

// ====================================================================================================================
//  FCk_Iskm_AnimationBuffer
// ====================================================================================================================
void
    FCk_Iskm_AnimationBuffer::
    InitRHI(FRHICommandListBase& RHICmdList)
{
    const uint32 ByteSize = Matrices.GetResourceDataSize();
    if (ByteSize == 0)
    { return; }

    const FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::CreateVertex(TEXT("CkIskm_AnimationBuffer"), ByteSize)
        .AddUsage(EBufferUsageFlags::ShaderResource)
        .SetInitActionResourceArray(&Matrices)
        .DetermineInitialState();
    Buffer = RHICmdList.CreateBuffer(Desc);

    const FRHIViewDesc::FBufferSRV::FInitializer ViewDesc = FRHIViewDesc::CreateBufferSRV()
        .SetType(FRHIViewDesc::EBufferType::Typed)
        .SetFormat(PF_A32B32G32R32F);
    ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(Buffer, ViewDesc);
}

void
    FCk_Iskm_AnimationBuffer::
    ReleaseRHI()
{
    ShaderResourceViewRHI.SafeRelease();
    Buffer.SafeRelease();
}

// ====================================================================================================================
//  FCk_Iskm_BoneIndexVertexBuffer
// ====================================================================================================================
void
    FCk_Iskm_BoneIndexVertexBuffer::
    Allocate()
{
    const int32 BytesPerIndex = bIs16BitBoneIndex ? 2 : 1;
    BoneIndexData.SetNumZeroed(NumVertices * MaxBoneInfluences * BytesPerIndex);
}

void
    FCk_Iskm_BoneIndexVertexBuffer::
    SetBoneIndex(int32 InVertexIdx, int32 InInfluenceIdx, uint32 InRenderBoneIdx)
{
    const int32 Slot = InVertexIdx * MaxBoneInfluences + InInfluenceIdx;
    if (bIs16BitBoneIndex)
    {
        reinterpret_cast<uint16*>(BoneIndexData.GetData())[Slot] = static_cast<uint16>(InRenderBoneIdx);
    }
    else
    {
        BoneIndexData[Slot] = static_cast<uint8>(InRenderBoneIdx);
    }
}

void
    FCk_Iskm_BoneIndexVertexBuffer::
    InitRHI(FRHICommandListBase& RHICmdList)
{
    const uint32 ByteSize = BoneIndexData.GetResourceDataSize();
    if (ByteSize == 0)
    { return; }

    const FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::CreateVertex(TEXT("CkIskm_BoneIndexBuffer"), ByteSize)
        .AddUsage(EBufferUsageFlags::Static)
        .SetInitActionResourceArray(&BoneIndexData)
        .DetermineInitialState();
    VertexBufferRHI = RHICmdList.CreateBuffer(Desc);
}

// ====================================================================================================================
//  FCk_Iskm_BatchedVertexFactory
// ====================================================================================================================
void
    FCk_Iskm_BatchedVertexFactory::
    InitRHI(FRHICommandListBase& RHICmdList)
{
    FDataType& D = Data;
    FVertexDeclarationElementList Elements;

    Elements.Add(AccessStreamComponent(D.PositionComponent, 0));
    Elements.Add(AccessStreamComponent(D.TangentBasisComponents[0], 1));
    Elements.Add(AccessStreamComponent(D.TangentBasisComponents[1], 2));

    if (D.TextureCoordinates.Num())
    {
        const uint8 BaseTexCoordAttribute = 5;
        for (int32 i = 0; i < D.TextureCoordinates.Num(); ++i)
        {
            Elements.Add(AccessStreamComponent(D.TextureCoordinates[i], BaseTexCoordAttribute + i));
        }
        for (int32 i = D.TextureCoordinates.Num(); i < MAX_TEXCOORDS; ++i)
        {
            Elements.Add(AccessStreamComponent(D.TextureCoordinates[D.TextureCoordinates.Num() - 1], BaseTexCoordAttribute + i));
        }
    }

    Elements.Add(AccessStreamComponent(D.ColorComponent, 13));
    Elements.Add(AccessStreamComponent(D.BoneIndices, 3));
    Elements.Add(AccessStreamComponent(D.BoneWeights, 4));

    if (D.MaxBoneInfluence > 4)
    {
        Elements.Add(AccessStreamComponent(D.ExtraBoneIndices, 14));
        Elements.Add(AccessStreamComponent(D.ExtraBoneWeights, 15));
    }

    AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 16, 0xff);

    InitDeclaration(Elements);
    check(GetDeclaration());
}

void
    FCk_Iskm_BatchedVertexFactory::
    FillData(const FCk_Iskm_BoneIndexVertexBuffer* InBoneIndexBuffer, const FSkeletalMeshLODRenderData* InLODData)
{
    const FStaticMeshVertexBuffers& SMVB = InLODData->StaticVertexBuffers;
    SMVB.PositionVertexBuffer.BindPositionVertexBuffer(this, Data);
    SMVB.StaticMeshVertexBuffer.BindTangentVertexBuffer(this, Data);
    SMVB.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(this, Data);
    SMVB.ColorVertexBuffer.BindColorVertexBuffer(this, Data);

    const FSkinWeightDataVertexBuffer* SkinData = InLODData->GetSkinWeightVertexBuffer()->GetDataVertexBuffer();
    const bool bExtra = Data.MaxBoneInfluence > 4;

    // Bone weights — borrowed unchanged from the source mesh skin-weight buffer.
    {
        const EVertexElementType ElemType = SkinData->Use16BitBoneWeight() ? VET_UShort4N : VET_UByte4N;
        const uint32 Stride = SkinData->GetConstantInfluencesVertexStride();
        Data.BoneWeights = FVertexStreamComponent(SkinData, SkinData->GetConstantInfluencesBoneWeightsOffset(), Stride, ElemType);
        if (bExtra)
        {
            Data.ExtraBoneWeights = Data.BoneWeights;
            Data.ExtraBoneWeights.Offset += (SkinData->GetBoneWeightByteSize() * 4);
        }
        else
        {
            Data.ExtraBoneWeights = FVertexStreamComponent(&GNullVertexBuffer, 0, 0, ElemType);
        }
    }

    // Bone indices — the custom remapped (render-bone space) buffer.
    {
        const uint32 Stride = (InBoneIndexBuffer->bIs16BitBoneIndex ? 2 : 1) * InBoneIndexBuffer->MaxBoneInfluences;
        const EVertexElementType ElemType = InBoneIndexBuffer->bIs16BitBoneIndex ? VET_UShort4 : VET_UByte4;
        Data.BoneIndices = FVertexStreamComponent(InBoneIndexBuffer, 0, Stride, ElemType);
        if (bExtra)
        {
            Data.ExtraBoneIndices = Data.BoneIndices;
            Data.ExtraBoneIndices.Offset = Stride / 2;
        }
        else
        {
            Data.ExtraBoneIndices = FVertexStreamComponent(&GNullVertexBuffer, 0, 0, ElemType);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------
//  Shader parameters — binds the AnimCollection uniform buffer (baked SRV + bone count) per draw.
// ----------------------------------------------------------------------------------------------------------------
class FCk_Iskm_BatchedShaderParameters : public FVertexFactoryShaderParameters
{
    DECLARE_TYPE_LAYOUT(FCk_Iskm_BatchedShaderParameters, NonVirtual);

public:
    void Bind(const FShaderParameterMap& ParameterMap) {}

    void GetElementShaderBindings(
        const FSceneInterface* Scene,
        const FSceneView* View,
        const FMeshMaterialShader* Shader,
        const EVertexInputStreamType InputStreamType,
        ERHIFeatureLevel::Type FeatureLevel,
        const FVertexFactory* VertexFactory,
        const FMeshBatchElement& BatchElement,
        FMeshDrawSingleShaderBindings& ShaderBindings,
        FVertexInputStreamArray& VertexStreams) const
    {
        const FCk_Iskm_BatchedVertexFactory* VF = static_cast<const FCk_Iskm_BatchedVertexFactory*>(VertexFactory);
        ShaderBindings.Add(Shader->GetUniformBufferParameter<FCk_Iskm_AnimCollectionUniformParams>(), VF->AnimCollectionUB);
    }
};

IMPLEMENT_TYPE_LAYOUT(FCk_Iskm_BatchedShaderParameters);

// ----------------------------------------------------------------------------------------------------------------
//  Permutation methods + type registration.
// ----------------------------------------------------------------------------------------------------------------
template<int MBI>
bool
    TCk_Iskm_BatchedVertexFactory<MBI>::
    ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
    return Parameters.MaterialParameters.bIsUsedWithSkeletalMesh || Parameters.MaterialParameters.bIsSpecialEngineMaterial;
}

template<int MBI>
void
    TCk_Iskm_BatchedVertexFactory<MBI>::
    ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
    OutEnvironment.SetDefine(TEXT("MAX_BONE_INFLUENCE"), MBI);
    // Force the per-instance custom-data read on (ENABLE_PER_INSTANCE_CUSTOM_DATA = USES || VF_REQUIRES). The .ush
    // runtime-selects per-instance vs per-component (CustomPrimitiveData[0]) by CustomDataOffset validity.
    OutEnvironment.SetDefine(TEXT("VF_REQUIRES_PER_INSTANCE_CUSTOM_DATA"), 1);
    OutEnvironment.SetDefine(TEXT("USES_PER_INSTANCE_CUSTOM_DATA"), 1);
    OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), 1);
    OutEnvironment.SetDefine(TEXT("USE_INSTANCING"), 1);
    OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), 0);
    OutEnvironment.SetDefine(TEXT("USE_DITHERED_LOD_TRANSITION_FOR_INSTANCED"), 0);
    OutEnvironment.SetDefine(TEXT("USE_DITHERED_LOD_TRANSITION"), 0);
    OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), 0);
}

namespace ck_iskm
{
    constexpr EVertexFactoryFlags VFFlags =
        EVertexFactoryFlags::UsedWithMaterials |
        EVertexFactoryFlags::SupportsDynamicLighting |
        EVertexFactoryFlags::SupportsPrecisePrevWorldPos |
        EVertexFactoryFlags::SupportsPrimitiveIdStream;
}

#define IMPL_CKISKM_VF(MBI) \
    using CkIskmVF##MBI = TCk_Iskm_BatchedVertexFactory<MBI>; \
    IMPLEMENT_TEMPLATE_VERTEX_FACTORY_TYPE(template<>, CkIskmVF##MBI, "/CkIskmRenderer/CkIskm_BatchedVertexFactory.ush", ck_iskm::VFFlags); \
    IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(CkIskmVF##MBI, SF_Vertex, FCk_Iskm_BatchedShaderParameters); \
    IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(CkIskmVF##MBI, SF_Pixel, FCk_Iskm_BatchedShaderParameters);

// MVP: 4-influence permutation only (mannequin).
IMPL_CKISKM_VF(4)

// ====================================================================================================================
//  FCk_Iskm_BatchedMeshData
// ====================================================================================================================
void
    FCk_Iskm_BatchedMeshData::
    InitFromMesh(USkeletalMesh* InMesh, USkeleton* InSkeleton, const TArray<int32>& InSkeletonBoneToRenderBone, ERHIFeatureLevel::Type InFeatureLevel)
{
    if (InMesh == nullptr || InSkeleton == nullptr)
    { return; }

    const FSkeletalMeshRenderData* RenderData = InMesh->GetResourceForRendering();
    if (RenderData == nullptr)
    { return; }

    SourceMesh = InMesh;
    BaseLOD = 0;

    for (int32 LODIndex = BaseLOD; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
    {
        const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
        const FSkinWeightVertexBuffer* SkinVB = LODData.GetSkinWeightVertexBuffer();

        // The MVP >4-bone-influence guard (dropped weights are not renormalized) lives in the CkCore-linked caller
        // UCk_IskmAnimCollection_Data::EnsureRenderResources as a CK_ENSURE_IF_NOT. This module is engine-only
        // (PostConfigInit, no Ck deps) so it can't CK_ENSURE here.

        FLODData* OutPtr = new FLODData();
        LODs.Add(OutPtr);
        FLODData& Out = *OutPtr;

        Out.BoneIndexBuffer.bIs16BitBoneIndex = SkinVB->Use16BitBoneIndex() || InSkeletonBoneToRenderBone.Num() > 255;
        Out.BoneIndexBuffer.MaxBoneInfluences = 4;
        Out.BoneIndexBuffer.NumVertices = SkinVB->GetNumVertices();
        Out.BoneIndexBuffer.Allocate();

        for (uint32 VertexIndex = 0; VertexIndex < SkinVB->GetNumVertices(); ++VertexIndex)
        {
            int32 SectionIndex = 0;
            int32 SectionVertexIndex = 0;
            LODData.GetSectionFromVertexIndex(VertexIndex, SectionIndex, SectionVertexIndex);
            const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

            const int32 NumInf = FMath::Min(4, static_cast<int32>(Section.MaxBoneInfluences));
            for (int32 InfluenceIndex = 0; InfluenceIndex < NumInf; ++InfluenceIndex)
            {
                const uint32 SectionBoneIndex = SkinVB->GetBoneIndex(VertexIndex, InfluenceIndex);
                if (!Section.BoneMap.IsValidIndex(SectionBoneIndex))
                { continue; }
                const FBoneIndexType MeshBoneIndex = Section.BoneMap[SectionBoneIndex];
                const int32 SkelBoneIndex = InSkeleton->GetSkeletonBoneIndexFromMeshBoneIndex(InMesh, MeshBoneIndex);
                int32 RenderBoneIndex = 0;
                if (SkelBoneIndex != INDEX_NONE && InSkeletonBoneToRenderBone.IsValidIndex(SkelBoneIndex))
                {
                    const int32 Mapped = InSkeletonBoneToRenderBone[SkelBoneIndex];
                    RenderBoneIndex = Mapped != INDEX_NONE ? Mapped : 0;
                }
                Out.BoneIndexBuffer.SetBoneIndex(VertexIndex, InfluenceIndex, static_cast<uint32>(RenderBoneIndex));
            }
        }

        Out.VertexFactory = MakeUnique<TCk_Iskm_BatchedVertexFactory<4>>(InFeatureLevel);
    }
}

void
    FCk_Iskm_BatchedMeshData::
    InitResources(FRHICommandListBase& RHICmdList, FRHIUniformBuffer* InAnimCollectionUB)
{
    if (SourceMesh == nullptr)
    { return; }
    const FSkeletalMeshRenderData* RenderData = SourceMesh->GetResourceForRendering();
    if (RenderData == nullptr)
    { return; }

    for (int32 i = 0; i < LODs.Num(); ++i)
    {
        const int32 LODIndex = BaseLOD + i;
        if (!RenderData->LODRenderData.IsValidIndex(LODIndex))
        { continue; }
        const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];

        FLODData& LOD = LODs[i];
        LOD.BoneIndexBuffer.InitResource(RHICmdList);
        if (LOD.VertexFactory.IsValid())
        {
            LOD.VertexFactory->AnimCollectionUB = InAnimCollectionUB;
            LOD.VertexFactory->FillData(&LOD.BoneIndexBuffer, &LODData);
            LOD.VertexFactory->InitResource(RHICmdList);
        }
    }
}

void
    FCk_Iskm_BatchedMeshData::
    ReleaseResources()
{
    for (FLODData& LOD : LODs)
    {
        LOD.BoneIndexBuffer.ReleaseResource();
        if (LOD.VertexFactory.IsValid())
        {
            LOD.VertexFactory->ReleaseResource();
        }
    }
}

FCk_Iskm_BatchedVertexFactory*
    FCk_Iskm_BatchedMeshData::
    Get_VertexFactory(int32 InLODIndex) const
{
    const int32 i = InLODIndex - BaseLOD;
    if (!LODs.IsValidIndex(i))
    { return nullptr; }
    return LODs[i].VertexFactory.Get();
}
