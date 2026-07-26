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

// ----------------------------------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------------------------------
void
    FCk_Iskm_BoneWeightVertexBuffer::
    Allocate()
{
    WeightData.SetNumZeroed(NumVertices * MaxBoneInfluences);
}

void
    FCk_Iskm_BoneWeightVertexBuffer::
    SetWeight(int32 InVertexIdx, int32 InInfluenceIdx, uint8 InWeight)
{
    WeightData[InVertexIdx * MaxBoneInfluences + InInfluenceIdx] = InWeight;
}

void
    FCk_Iskm_BoneWeightVertexBuffer::
    InitRHI(FRHICommandListBase& RHICmdList)
{
    const uint32 ByteSize = WeightData.GetResourceDataSize();
    if (ByteSize == 0)
    { return; }

    const FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::CreateVertex(TEXT("CkIskm_BoneWeightBuffer"), ByteSize)
        .AddUsage(EBufferUsageFlags::Static)
        .SetInitActionResourceArray(&WeightData)
        .DetermineInitialState();
    VertexBufferRHI = RHICmdList.CreateBuffer(Desc);
}

// ----------------------------------------------------------------------------------------------------
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
    FillData(const FCk_Iskm_BoneIndexVertexBuffer* InBoneIndexBuffer, const FCk_Iskm_BoneWeightVertexBuffer* InBoneWeightBuffer, const FSkeletalMeshLODRenderData* InLODData)
{
    const FStaticMeshVertexBuffers& SMVB = InLODData->StaticVertexBuffers;
    SMVB.PositionVertexBuffer.BindPositionVertexBuffer(this, Data);
    SMVB.StaticMeshVertexBuffer.BindTangentVertexBuffer(this, Data);
    SMVB.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(this, Data);
    SMVB.ColorVertexBuffer.BindColorVertexBuffer(this, Data);

    const bool bExtra = Data.MaxBoneInfluence > 4;

    {
        const uint32 Stride = InBoneWeightBuffer->MaxBoneInfluences;
        Data.BoneWeights = FVertexStreamComponent(InBoneWeightBuffer, 0, Stride, VET_UByte4N);
        if (bExtra)
        {
            Data.ExtraBoneWeights = Data.BoneWeights;
            Data.ExtraBoneWeights.Offset = 4;
        }
        else
        {
            Data.ExtraBoneWeights = FVertexStreamComponent(&GNullVertexBuffer, 0, 0, VET_UByte4N);
        }
    }

    {
        const uint32 BytesPerIndex = InBoneIndexBuffer->bIs16BitBoneIndex ? 2 : 1;
        const uint32 Stride = BytesPerIndex * InBoneIndexBuffer->MaxBoneInfluences;
        const EVertexElementType ElemType = InBoneIndexBuffer->bIs16BitBoneIndex ? VET_UShort4 : VET_UByte4;
        Data.BoneIndices = FVertexStreamComponent(InBoneIndexBuffer, 0, Stride, ElemType);
        if (bExtra)
        {
            Data.ExtraBoneIndices = Data.BoneIndices;
            Data.ExtraBoneIndices.Offset = BytesPerIndex * 4; // influences 4..7 follow 0..3 per vertex
        }
        else
        {
            Data.ExtraBoneIndices = FVertexStreamComponent(&GNullVertexBuffer, 0, 0, ElemType);
        }
    }
}

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
        ShaderBindings.Add(Shader->GetUniformBufferParameter<FCk_Iskm_AnimCollectionUniformParams>(), VF->AnimCollectionUB.GetReference());
    }
};

IMPLEMENT_TYPE_LAYOUT(FCk_Iskm_BatchedShaderParameters);

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

IMPL_CKISKM_VF(4)
IMPL_CKISKM_VF(8)

// ----------------------------------------------------------------------------------------------------
void
    FCk_Iskm_BatchedMeshData::
    InitFromMesh(USkeletalMesh* InMesh, USkeleton* InSkeleton, const TArray<int32>& InSkeletonBoneToRenderBone, ERHIFeatureLevel::Type InFeatureLevel)
{
    if (InMesh == nullptr || InSkeleton == nullptr)
    { return; }

    const FSkeletalMeshRenderData* RenderData = InMesh->GetResourceForRendering();
    if (RenderData == nullptr)
    { return; }

    SourceRenderData = RenderData;
    BaseLOD = 0;
    NumBoneRemapMisses = 0;

    for (int32 LODIndex = BaseLOD; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
    {
        const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
        const FSkinWeightVertexBuffer* SkinVB = LODData.GetSkinWeightVertexBuffer();

        FLODData* OutPtr = new FLODData();
        LODs.Add(OutPtr);
        FLODData& Out = *OutPtr;

        // 4 or 8 influences per vertex, matched to the source (>8 keeps the strongest 8; weights renormalize).
        const int32 MBI = (static_cast<int32>(SkinVB->GetMaxBoneInfluences()) > 4) ? 8 : 4;

        Out.BoneIndexBuffer.bIs16BitBoneIndex = SkinVB->Use16BitBoneIndex() || InSkeletonBoneToRenderBone.Num() > 255;
        Out.BoneIndexBuffer.MaxBoneInfluences = MBI;
        Out.BoneIndexBuffer.NumVertices = SkinVB->GetNumVertices();
        Out.BoneIndexBuffer.Allocate();

        Out.BoneWeightBuffer.MaxBoneInfluences = MBI;
        Out.BoneWeightBuffer.NumVertices = SkinVB->GetNumVertices();
        Out.BoneWeightBuffer.Allocate();

        for (uint32 VertexIndex = 0; VertexIndex < SkinVB->GetNumVertices(); ++VertexIndex)
        {
            int32 SectionIndex = 0;
            int32 SectionVertexIndex = 0;
            LODData.GetSectionFromVertexIndex(VertexIndex, SectionIndex, SectionVertexIndex);
            const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

            uint32 VertexWeightOffset = 0;
            uint32 VertexInfluenceCount = 0;
            SkinVB->GetVertexInfluenceOffsetCount(VertexIndex, VertexWeightOffset, VertexInfluenceCount);

            const int32 NumInf = FMath::Min(MBI, static_cast<int32>(VertexInfluenceCount));

            uint16 RawWeights[8] = { 0 };
            uint32 WeightSum = 0;
            for (int32 InfluenceIndex = 0; InfluenceIndex < NumInf; ++InfluenceIndex)
            {
                RawWeights[InfluenceIndex] = SkinVB->GetBoneWeight(VertexIndex, InfluenceIndex);
                WeightSum += RawWeights[InfluenceIndex];
            }

            // Weights must sum to EXACTLY 255 — a sub-unit sum visibly shrinks verts — so the residual is
            // handed to the strongest influence below. Remap misses only aggregate into NumBoneRemapMisses
            // (hot loop, and this engine-only module can't ensure); EnsureRenderResources ensures on them.
            int32 StrongestSlot = INDEX_NONE;
            int32 QuantizedSum = 0;
            for (int32 InfluenceIndex = 0; InfluenceIndex < NumInf; ++InfluenceIndex)
            {
                const uint32 SectionBoneIndex = SkinVB->GetBoneIndex(VertexIndex, InfluenceIndex);
                if (Section.BoneMap.IsValidIndex(SectionBoneIndex))
                {
                    const FBoneIndexType MeshBoneIndex = Section.BoneMap[SectionBoneIndex];
                    const int32 SkelBoneIndex = InSkeleton->GetSkeletonBoneIndexFromMeshBoneIndex(InMesh, MeshBoneIndex);
                    int32 RenderBoneIndex = 0;
                    if (SkelBoneIndex != INDEX_NONE && InSkeletonBoneToRenderBone.IsValidIndex(SkelBoneIndex))
                    {
                        const int32 Mapped = InSkeletonBoneToRenderBone[SkelBoneIndex];
                        RenderBoneIndex = Mapped != INDEX_NONE ? Mapped : 0;
                        if (Mapped == INDEX_NONE)
                        { ++NumBoneRemapMisses; } // bone outside the bake's render-bone set: rigid-bind to root
                    }
                    else
                    { ++NumBoneRemapMisses; } // mesh bone missing from the skeleton/remap table: rigid-bind to root
                    Out.BoneIndexBuffer.SetBoneIndex(VertexIndex, InfluenceIndex, static_cast<uint32>(RenderBoneIndex));

                    const uint8 Quantized = (WeightSum > 0)
                        ? static_cast<uint8>(FMath::Clamp<uint32>((static_cast<uint32>(RawWeights[InfluenceIndex]) * 255 + WeightSum / 2) / WeightSum, 0, 255))
                        : (InfluenceIndex == 0 ? 255 : 0); // degenerate source: rigid-bind to influence 0
                    Out.BoneWeightBuffer.SetWeight(VertexIndex, InfluenceIndex, Quantized);
                    QuantizedSum += Quantized;
                    if (StrongestSlot == INDEX_NONE || RawWeights[InfluenceIndex] > RawWeights[StrongestSlot])
                    { StrongestSlot = InfluenceIndex; }
                }
                else
                {
                    // Corrupt source: section-local bone index outside the section's BoneMap. Drop the influence
                    // entirely — zero its weight so the index (root) and weight streams stay consistent; the
                    // residual fix below redistributes its mass to the strongest VALID influence.
                    ++NumBoneRemapMisses;
                    Out.BoneWeightBuffer.SetWeight(VertexIndex, InfluenceIndex, 0);
                }
            }
            if (StrongestSlot != INDEX_NONE && WeightSum > 0 && QuantizedSum != 255)
            {
                const int32 Fixed = static_cast<int32>(Out.BoneWeightBuffer.WeightData[VertexIndex * MBI + StrongestSlot]) + (255 - QuantizedSum);
                Out.BoneWeightBuffer.SetWeight(VertexIndex, StrongestSlot, static_cast<uint8>(FMath::Clamp(Fixed, 0, 255)));
            }
        }

        Out.VertexFactory = (MBI == 8)
            ? TUniquePtr<FCk_Iskm_BatchedVertexFactory>(MakeUnique<TCk_Iskm_BatchedVertexFactory<8>>(InFeatureLevel))
            : TUniquePtr<FCk_Iskm_BatchedVertexFactory>(MakeUnique<TCk_Iskm_BatchedVertexFactory<4>>(InFeatureLevel));
    }
}

void
    FCk_Iskm_BatchedMeshData::
    InitResources(FRHICommandListBase& RHICmdList, FRHIUniformBuffer* InAnimCollectionUB)
{
    // Game-thread snapshot — never deref a UObject here (lifetime contract on the member in the header)
    const FSkeletalMeshRenderData* RenderData = SourceRenderData;
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
        LOD.BoneWeightBuffer.InitResource(RHICmdList);
        if (LOD.VertexFactory.IsValid())
        {
            LOD.VertexFactory->AnimCollectionUB = InAnimCollectionUB;
            LOD.VertexFactory->FillData(&LOD.BoneIndexBuffer, &LOD.BoneWeightBuffer, &LODData);
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
        LOD.BoneWeightBuffer.ReleaseResource();
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
