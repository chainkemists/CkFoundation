#pragma once

#include "CoreMinimal.h"

#include "RenderResource.h"
#include "VertexFactory.h"
#include "Components.h"
#include "ShaderParameterMacros.h"
#include "RHIDefinitions.h"
#include "Containers/DynamicRHIResourceArray.h"

#include "CkIskmRendererVF/CkIskm_BoneMatrix.h"

// ====================================================================================================================
//  CkIskmRenderer — render-thread resources for batched GPU-skinned skeletal instancing.
//  Port of Skelot v6 SkelotRenderResources.h, GPUScene desktop path only (HP float32, no manual-vertex-fetch,
//  no legacy/non-GPUScene path, no curves). See Shaders/CkIskmRenderer/CkIskm_BatchedVertexFactory.ush.
//
//  Lives in the engine-only PostConfigInit module CkIskmRendererVF so the FVertexFactory type registers before
//  the engine seals the vertex-factory list. API shapes are verified against the engine render headers.
// ====================================================================================================================

class USkeletalMesh;
class USkeleton;

// Per-AnimCollection uniform buffer: binds the baked bone-matrix SRV + bone stride. Shader name "CkIskmAC".
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FCk_Iskm_AnimCollectionUniformParams, CKISKMRENDERERVF_API)
    SHADER_PARAMETER(uint32, BoneCount)
    SHADER_PARAMETER_SRV(Buffer<float4>, AnimationBuffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FCk_Iskm_AnimCollectionUniformParams> FCk_Iskm_AnimCollectionUniformParamsRef;

// ----------------------------------------------------------------------------------------------------------------
//  Baked bone-matrix GPU buffer — a typed Buffer<float4> SRV holding every baked frame's transposed 3x4 matrices,
//  flattened: matrixIndex = frame * RenderBoneCount + bone, each = 3 x float4. HP float32 (PF_A32B32G32R32F).
// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERERVF_API FCk_Iskm_AnimationBuffer : public FRenderResource
{
public:
    TResourceArray<FCk_Iskm_BoneMatrix3x4> Matrices;
    FBufferRHIRef Buffer;
    FShaderResourceViewRHIRef ShaderResourceViewRHI;

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual void ReleaseRHI() override;
    virtual FString GetFriendlyName() const override { return TEXT("FCk_Iskm_AnimationBuffer"); }
};

// ----------------------------------------------------------------------------------------------------------------
//  Per-mesh bone-index vertex stream — remaps each vertex's skin-weight bone slots into render-bone space.
//  8- or 16-bit indices, 4 or 8 influences per vertex.
// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERERVF_API FCk_Iskm_BoneIndexVertexBuffer : public FVertexBuffer
{
public:
    TResourceArray<uint8> BoneIndexData;
    bool  bIs16BitBoneIndex = false;
    int32 MaxBoneInfluences = 4;
    int32 NumVertices = 0;

    void Allocate();
    void SetBoneIndex(int32 InVertexIdx, int32 InInfluenceIdx, uint32 InRenderBoneIdx);

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual FString GetFriendlyName() const override { return TEXT("FCk_Iskm_BoneIndexVertexBuffer"); }
};

// ----------------------------------------------------------------------------------------------------------------
//  Per-mesh bone-weight vertex stream — OWNED (not borrowed from the source mesh): weights are renormalized to
//  sum exactly 1 over the kept influences and quantized to 8-bit unorm, laid out at exactly MaxBoneInfluences
//  per vertex. Owning the layout removes every assumption about the source buffer (variable vs constant
//  influence layout, 16-bit weights, influence counts that aren't 4/8) — the failure modes of the previous
//  borrowed-stream approach.
// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERERVF_API FCk_Iskm_BoneWeightVertexBuffer : public FVertexBuffer
{
public:
    TResourceArray<uint8> WeightData; // unorm, NumVertices * MaxBoneInfluences
    int32 MaxBoneInfluences = 4;
    int32 NumVertices = 0;

    void Allocate();
    void SetWeight(int32 InVertexIdx, int32 InInfluenceIdx, uint8 InWeight);

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    virtual FString GetFriendlyName() const override { return TEXT("FCk_Iskm_BoneWeightVertexBuffer"); }
};

// ----------------------------------------------------------------------------------------------------------------
//  The batched GPU linear-blend-skinning vertex factory.
// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERERVF_API FCk_Iskm_BatchedVertexFactory : public FVertexFactory
{
public:
    struct FDataType : FStaticMeshDataType
    {
        FVertexStreamComponent BoneIndices;
        FVertexStreamComponent BoneWeights;
        FVertexStreamComponent ExtraBoneIndices;
        FVertexStreamComponent ExtraBoneWeights;
        int32 MaxBoneInfluence = 4;
    };

    FCk_Iskm_BatchedVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
        : FVertexFactory(InFeatureLevel) {}

    FDataType Data;
    FRHIUniformBuffer* AnimCollectionUB = nullptr;

    virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
    void FillData(const FCk_Iskm_BoneIndexVertexBuffer* InBoneIndexBuffer, const FCk_Iskm_BoneWeightVertexBuffer* InBoneWeightBuffer, const class FSkeletalMeshLODRenderData* InLODData);
};

template<int MBI>
class TCk_Iskm_BatchedVertexFactory : public FCk_Iskm_BatchedVertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE(TCk_Iskm_BatchedVertexFactory);

public:
    TCk_Iskm_BatchedVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
        : FCk_Iskm_BatchedVertexFactory(InFeatureLevel) { Data.MaxBoneInfluence = MBI; }

    static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
    static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
    static void ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors) {}
};

// ----------------------------------------------------------------------------------------------------------------
//  Per-mesh render data: one entry per LOD, each owning the remapped bone-index buffer + the vertex factory.
// ----------------------------------------------------------------------------------------------------------------
class CKISKMRENDERERVF_API FCk_Iskm_BatchedMeshData
{
public:
    // Non-copyable: FLODData owns a TUniquePtr<vertex factory> + an FVertexBuffer (both non-copyable).
    FCk_Iskm_BatchedMeshData() = default;
    FCk_Iskm_BatchedMeshData(const FCk_Iskm_BatchedMeshData&) = delete;
    FCk_Iskm_BatchedMeshData& operator=(const FCk_Iskm_BatchedMeshData&) = delete;

    struct FLODData
    {
        FCk_Iskm_BoneIndexVertexBuffer BoneIndexBuffer;
        FCk_Iskm_BoneWeightVertexBuffer BoneWeightBuffer;
        TUniquePtr<FCk_Iskm_BatchedVertexFactory> VertexFactory;
    };

    TIndirectArray<FLODData> LODs;
    int32 BaseLOD = 0;
    USkeletalMesh* SourceMesh = nullptr;

    // CPU build of the per-LOD remapped bone indices. InSkeletonBoneToRenderBone maps skeleton-bone -> render-bone
    // (from the baker); it decouples this engine-only module from the AnimCollection asset.
    void InitFromMesh(USkeletalMesh* InMesh, USkeleton* InSkeleton, const TArray<int32>& InSkeletonBoneToRenderBone, ERHIFeatureLevel::Type InFeatureLevel);
    void InitResources(FRHICommandListBase& RHICmdList, FRHIUniformBuffer* InAnimCollectionUB);
    void ReleaseResources();

    FCk_Iskm_BatchedVertexFactory* Get_VertexFactory(int32 InLODIndex) const;
};

// ----------------------------------------------------------------------------------------------------------------
//  Per-AnimCollection render-data aggregate (held transiently by UCk_IskmAnimCollection_Data via a TPimplPtr).
// ----------------------------------------------------------------------------------------------------------------
struct CKISKMRENDERERVF_API FCk_Iskm_BatchedRenderData
{
    FCk_Iskm_BatchedRenderData() = default;
    FCk_Iskm_BatchedRenderData(const FCk_Iskm_BatchedRenderData&) = delete;
    FCk_Iskm_BatchedRenderData& operator=(const FCk_Iskm_BatchedRenderData&) = delete;

    FCk_Iskm_AnimationBuffer AnimationBuffer;
    FCk_Iskm_AnimCollectionUniformParamsRef AnimCollectionUB;
    FCk_Iskm_BatchedMeshData DefaultMeshData;
};
