#include "CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmAnimCollection_BakedPose.h"
#include "CkIskmRendererVF/CkIskm_BatchedRenderResources.h"

#include "CkAnimation/AnimBake/CkAnimBake.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "RenderingThread.h"
#include "RenderUtils.h"
#include "RHIGlobals.h"
#include "Misc/App.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
#endif

auto
    UCk_IskmAnimCollection_Data::
    Find_SequenceIndex_ByAsset(const UAnimSequenceBase* InAsset) const
    -> int32
{
    return _Sequences.IndexOfByPredicate([&](const FCk_IskmAnimCollection_SequenceDef& E)
    {
        return E.Get_Sequence() == InAsset;
    });
}

// ====================================================================================================================
//  Plan-2 CPU bone-matrix bake. Sampling/compaction/layout/bounds live in the shared ck::anim_bake core
//  (CkAnimation/AnimBake — extracted from this function's original Skelot port, also consumed by CkVat);
//  this function keeps only the Iskm-specific OUTPUT ENCODING: transposed 3x4 bone matrices in a flat
//  Buffer<float4>-ready array. CPU-only, no RHI.
// ====================================================================================================================
auto
    UCk_IskmAnimCollection_Data::
    Build_BakedPoseData()
    -> bool
{
    if (ck::Is_NOT_Valid(_Skeleton) || ck::Is_NOT_Valid(_DefaultMesh))
    { return false; }

    FCk_AnimBake_SampleParams SampleParams;
    SampleParams.ExtractRootMotion = _ExtractRootMotion;
    SampleParams.DisableRetargeting = _DisableRetargeting;

    const auto SkeletonData = ck::anim_bake::BuildSkeletonData(*_Skeleton, *_DefaultMesh, SampleParams);
    if (NOT SkeletonData.IsSet())
    { return false; }

    TArray<UAnimSequenceBase*> SequenceAssets;
    SequenceAssets.Reserve(_Sequences.Num());
    for (const FCk_IskmAnimCollection_SequenceDef& Def : _Sequences)
    { SequenceAssets.Add(Def.Get_Sequence().Get()); }

    const auto Layout = ck::anim_bake::BuildFrameLayout(SequenceAssets, _SampleFrequency);

    auto Baked = MakePimpl<FCk_Iskm_BakedPose>();
    Baked->HighPrecision = true;
    Baked->RenderBoneCount = SkeletonData->RenderBoneCount;
    Baked->RenderRequiredBones = SkeletonData->RenderRequiredBones;
    Baked->SkeletonBoneToRenderBone = SkeletonData->SkeletonBoneToRenderBone;
    Baked->RefPoseInverse = SkeletonData->RefPoseInverse;

    Baked->Sequences.Reserve(Layout.Sequences.Num());
    for (const FCk_AnimBake_SequenceLayout& SeqLayout : Layout.Sequences)
    {
        FCk_Iskm_BakedSequence BakedSeq;
        BakedSeq.Sequence = SeqLayout.Sequence;
        BakedSeq.SampleFrequency = SeqLayout.SampleFrequency;
        BakedSeq.AnimationFrameCount = SeqLayout.FrameCount;
        BakedSeq.AnimationFrameIndex = SeqLayout.FrameIndex;
        Baked->Sequences.Add(BakedSeq);
    }
    Baked->FrameCountSequences = Layout.TotalFrameCount;
    Baked->TotalFrameCount = Layout.TotalFrameCount; // MVP: no transition / dynamic-pose region

    const int32 RenderBoneCount = Baked->RenderBoneCount;
    Baked->Matrices.SetNumUninitialized(RenderBoneCount * Baked->TotalFrameCount);

    // Per-frame culling bounds (Phase 4). MVP: the mesh's static bound, identical for every frame.
    const FBox3f MeshBound = static_cast<FBox3f>(_DefaultMesh->GetBounds().GetBox());
    Baked->FrameBounds.Init(MeshBound, Baked->FrameCountSequences);

    // ShaderMatrix[bone] = RefPoseInverse[bone] * ComponentSpaceBoneMatrix[bone], stored transposed 3x4.
    const auto CalcRenderMatrices = [&Baked, RenderBoneCount](TArrayView<const FTransform> InPoseComponentSpace, int32 InFrameIndex)
    {
        FCk_Iskm_BoneMatrix3x4* const Out = Baked->Matrices.GetData() + (InFrameIndex * RenderBoneCount);
        for (int32 i = 0; i < RenderBoneCount; ++i)
        {
            const int32 CompactBoneIdx = Baked->RenderRequiredBones[i];
            const FMatrix44f BoneMatrix = static_cast<FTransform3f>(InPoseComponentSpace[CompactBoneIdx]).ToMatrixWithScale();
            const FMatrix44f ShaderMatrix = Baked->RefPoseInverse[CompactBoneIdx] * BoneMatrix;

            // transposed 3x4 store (matches SkelotSetMatrix3x4Transpose; shader uses mul(Matrix, Vector)).
            const float* RESTRICT Src = &ShaderMatrix.M[0][0];
            float* RESTRICT Dst = Out[i].M;
            Dst[0] = Src[0]; Dst[1] = Src[4]; Dst[2]  = Src[8];  Dst[3]  = Src[12];
            Dst[4] = Src[1]; Dst[5] = Src[5]; Dst[6]  = Src[9];  Dst[7]  = Src[13];
            Dst[8] = Src[2]; Dst[9] = Src[6]; Dst[10] = Src[10]; Dst[11] = Src[14];
        }
    };

    // Frame 0 (reference pose, yielding identity matrices) + every sequence frame; render-bone translations
    // accumulate inside the core and feed the conservative animated bounds.
    const FBox BoneBoundsAllFrames =
        ck::anim_bake::SamplePoses(*_Skeleton, *SkeletonData, Layout, SampleParams, CalcRenderMatrices);

    Baked->AnimatedBounds = ck::anim_bake::ComputeAnimatedBounds(*SkeletonData, BoneBoundsAllFrames, *_DefaultMesh);

    Baked->IsBaked = true;
    _BakedPose = MoveTemp(Baked);
    return true;
}

auto
    UCk_IskmAnimCollection_Data::
    Get_AnimatedMeshBounds() const
    -> FBox
{
    if (Get_IsBaked() && _BakedPose->AnimatedBounds.IsValid != 0)
    { return _BakedPose->AnimatedBounds; }
    if (ck::IsValid(_DefaultMesh))
    { return _DefaultMesh->GetBounds().GetBox(); }
    return FBox(FVector(-50.0), FVector(50.0));
}

auto
    UCk_IskmAnimCollection_Data::
    Get_BakedPose() const
    -> const FCk_Iskm_BakedPose*
{
    return _BakedPose.Get();
}

auto
    UCk_IskmAnimCollection_Data::
    Get_IsBaked() const
    -> bool
{
    return ck::IsValid(_BakedPose.Get(), ck::IsValid_Policy_NullptrOnly{}) && _BakedPose->IsBaked;
}

// ====================================================================================================================
//  Plan-2 GPU render resources
// ====================================================================================================================
auto
    UCk_IskmAnimCollection_Data::
    EnsureRenderResources()
    -> void
{
    if (FApp::CanEverRender() == false)
    { return; } // headless / -nullrhi: CPU bake only, no GPU resources

    if (Get_IsBaked() == false)
    {
        if (Build_BakedPoseData() == false)
        { return; }
    }

    if (ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const FCk_Iskm_BakedPose* Baked = Get_BakedPose();
    if (Baked == nullptr || Baked->Matrices.Num() == 0)
    { return; }

    _RenderData = MakePimpl<FCk_Iskm_BatchedRenderData>();
    FCk_Iskm_BatchedRenderData* RD = _RenderData.Get();

    // CPU: copy the baked matrices into the SRV-backing resource array.
    RD->AnimationBuffer.Matrices.SetNumUninitialized(Baked->Matrices.Num());
    FMemory::Memcpy(RD->AnimationBuffer.Matrices.GetData(), Baked->Matrices.GetData(),
        Baked->Matrices.Num() * sizeof(FCk_Iskm_BoneMatrix3x4));

    // CPU: build the default mesh's render data (render-bone remap + vertex factories). Pass the skeleton +
    // remap table directly so the engine-only VF module stays decoupled from this asset type.
    if (ck::IsValid(_DefaultMesh) && ck::IsValid(_Skeleton))
    {
        // Bone-influence guard. Policy lives here — the CkCore-linked module — because the engine-only VF module
        // (PostConfigInit, no Ck deps) can't CK_ENSURE. The batched path skins up to 8 influences (4- and
        // 8-influence vertex factories, weights renormalized into an owned buffer); beyond 8 the strongest 8 are
        // kept and renormalized — visually close, but flag the content.
        if (const FSkeletalMeshRenderData* RenderData = _DefaultMesh->GetResourceForRendering())
        {
            for (const FSkeletalMeshLODRenderData& LODData : RenderData->LODRenderData)
            {
                for (const FSkelMeshRenderSection& Section : LODData.RenderSections)
                {
                    CK_ENSURE_IF_NOT(Section.MaxBoneInfluences <= 8,
                        TEXT("[CkIskm] Batched renderer supports <= 8 bone influences; a section has {} — strongest 8 kept, weights renormalized."),
                        Section.MaxBoneInfluences)
                    { }
                }

                // The bone-index/weight build reads skin weights on the CPU (FSkinWeightVertexBuffer accessors).
                // Cooked builds discard that CPU copy unless the mesh keeps CPU access — the buffers would build
                // from garbage. Flag the content so BusterBlock crowd meshes get bNeedsCPUAccess set.
                CK_ENSURE_IF_NOT(GIsEditor || LODData.GetSkinWeightVertexBuffer()->GetNeedsCPUAccess(),
                    TEXT("[CkIskm] Batched renderer requires CPU-accessible skin weights in cooked builds — set bNeedsCPUAccess on mesh [{}]"),
                    _DefaultMesh)
                { }
            }
        }

        RD->DefaultMeshData.InitFromMesh(_DefaultMesh, _Skeleton, Baked->SkeletonBoneToRenderBone, GMaxRHIFeatureLevel);
    }

    const uint32 BoneCount = static_cast<uint32>(Baked->RenderBoneCount);

    ENQUEUE_RENDER_COMMAND(CkIskm_InitAnimCollectionResources)(
        [RD, BoneCount](FRHICommandListImmediate& RHICmdList)
        {
            RD->AnimationBuffer.InitResource(RHICmdList);

            FCk_Iskm_AnimCollectionUniformParams Params;
            Params.BoneCount = BoneCount;
            Params.AnimationBuffer = RD->AnimationBuffer.ShaderResourceViewRHI;
            RD->AnimCollectionUB = FCk_Iskm_AnimCollectionUniformParamsRef::CreateUniformBufferImmediate(Params, UniformBuffer_MultiFrame);

            RD->DefaultMeshData.InitResources(RHICmdList, RD->AnimCollectionUB.GetReference());
        });
}

auto
    UCk_IskmAnimCollection_Data::
    Get_DefaultMeshData() const
    -> const FCk_Iskm_BatchedMeshData*
{
    return ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}) ? &_RenderData->DefaultMeshData : nullptr;
}

auto
    UCk_IskmAnimCollection_Data::
    ReleaseRenderResources()
    -> void
{
    if (NOT ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    // Game-thread ReleaseResource enqueues the render-thread ReleaseRHI; the _ReleaseResourcesFence (BeginDestroy)
    // gates UObject destruction until those complete, so the resource objects outlive their GPU release.
    _RenderData->DefaultMeshData.ReleaseResources();
    _RenderData->AnimationBuffer.ReleaseResource();
    _RenderData->AnimCollectionUB.SafeRelease();
}

auto
    UCk_IskmAnimCollection_Data::
    BeginDestroy()
    -> void
{
    Super::BeginDestroy();
    ReleaseRenderResources();
    _ReleaseResourcesFence.BeginFence();
}

auto
    UCk_IskmAnimCollection_Data::
    IsReadyForFinishDestroy()
    -> bool
{
    return Super::IsReadyForFinishDestroy() && _ReleaseResourcesFence.IsFenceComplete();
}

#if WITH_EDITOR
auto
    UCk_IskmAnimCollection_Data::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);
}

auto
    UCk_IskmAnimCollection_Data::
    IsDataValid(FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (ck::Is_NOT_Valid(_Skeleton))
    {
        InContext.AddError(FText::FromString(TEXT("AnimCollection has no Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::IsValid(_DefaultMesh) && _DefaultMesh->GetSkeleton() != _Skeleton)
    {
        InContext.AddError(FText::FromString(TEXT("DefaultMesh skeleton does not match the AnimCollection's Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    for (auto Index = 0; Index < _Sequences.Num(); ++Index)
    {
        const auto& Def = _Sequences[Index];
        const auto* Seq = Def.Get_Sequence().Get();
        if (ck::Is_NOT_Valid(Seq))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) is null."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (Seq->GetSkeleton() != _Skeleton)
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) skeleton mismatch."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
