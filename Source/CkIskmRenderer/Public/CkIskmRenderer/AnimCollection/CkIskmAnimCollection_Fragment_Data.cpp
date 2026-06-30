#include "CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmAnimCollection_BakedPose.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimCurveFilter.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
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
//  Plan-2 CPU bone-matrix bake. Direct port of Skelot's USkelotAnimCollection::BuildAnimationData /
//  BuildSequence / CalcRenderMatrices (SkelotAnimCollection.cpp ~1246/1360/1630). CPU-only, no RHI.
// ====================================================================================================================
auto
    UCk_IskmAnimCollection_Data::
    Build_BakedPoseData()
    -> bool
{
    if (ck::Is_NOT_Valid(_Skeleton) || ck::Is_NOT_Valid(_DefaultMesh))
    { return false; }

    auto Baked = MakePimpl<FCk_Iskm_BakedPose>();
    Baked->bHighPrecision = true;

    // ---- 1. Render-bone compaction: the skinned subset of skeleton bones (union over the mesh's LOD ActiveBoneIndices) ----
    const FReferenceSkeleton& SkelRefSkeleton = _Skeleton->GetReferenceSkeleton();
    const int32 NumSkelBones = SkelRefSkeleton.GetNum();
    if (NumSkelBones == 0)
    { return false; }

    const FSkeletalMeshRenderData* RenderData = _DefaultMesh->GetResourceForRendering();
    if (RenderData == nullptr || RenderData->LODRenderData.Num() == 0)
    { return false; }

    TArray<bool> RequiredBones;
    RequiredBones.Init(false, NumSkelBones);
    for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
    {
        const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LODIndex];
        for (const FBoneIndexType MeshBoneIndex : LODRenderData.ActiveBoneIndices)
        {
            const int32 SkelBoneIndex = _Skeleton->GetSkeletonBoneIndexFromMeshBoneIndex(_DefaultMesh, MeshBoneIndex);
            if (SkelBoneIndex != INDEX_NONE)
            { RequiredBones[SkelBoneIndex] = true; }
        }
    }

    Baked->SkeletonBoneToRenderBone.Init(INDEX_NONE, NumSkelBones);
    Baked->RenderRequiredBones.Reserve(NumSkelBones);
    for (int32 SkelBoneIndex = 0; SkelBoneIndex < NumSkelBones; ++SkelBoneIndex)
    {
        if (RequiredBones[SkelBoneIndex])
        {
            Baked->SkeletonBoneToRenderBone[SkelBoneIndex] = Baked->RenderRequiredBones.Num();
            Baked->RenderRequiredBones.Add(static_cast<uint16>(SkelBoneIndex));
        }
    }
    const int32 RenderBoneCount = Baked->RenderRequiredBones.Num();
    if (RenderBoneCount == 0)
    { return false; }
    Baked->RenderBoneCount = RenderBoneCount;

    // ---- 2. Full-skeleton bone container (so compact-pose-bone-index == skeleton-bone-index) ----
    TArray<FBoneIndexType> AnimationRequiredBones;
    AnimationRequiredBones.SetNumUninitialized(NumSkelBones);
    for (int32 i = 0; i < NumSkelBones; ++i)
    { AnimationRequiredBones[i] = static_cast<FBoneIndexType>(i); }

    FBoneContainer BoneContainer;
    const UE::Anim::FCurveFilterSettings CurveFilter(UE::Anim::ECurveFilterMode::DisallowFiltered);
    BoneContainer.InitializeTo(MoveTemp(AnimationRequiredBones), CurveFilter, *_Skeleton);
    BoneContainer.SetDisableRetargeting(_bDisableRetargeting);
    // Force RAW data in editor: at PostLoad some sequences may not have finished compression.
    BoneContainer.SetUseRAWData(static_cast<bool>(WITH_EDITOR));

    const int32 NumContainerBones = BoneContainer.GetNumBones();

    // ---- 3. Component-space reference pose + its inverse (per skeleton bone) ----
    TArray<FTransform> RefPoseComponentSpace;
    RefPoseComponentSpace.SetNumUninitialized(NumContainerBones);
    Baked->RefPoseInverse.SetNumUninitialized(NumContainerBones);
    RefPoseComponentSpace[0] = BoneContainer.GetRefPoseTransform(FCompactPoseBoneIndex(0));
    Baked->RefPoseInverse[0] = static_cast<FTransform3f>(RefPoseComponentSpace[0]).Inverse().ToMatrixWithScale();
    for (FCompactPoseBoneIndex BoneIdx(1); BoneIdx < NumContainerBones; ++BoneIdx)
    {
        const FCompactPoseBoneIndex ParentIdx = BoneContainer.GetParentBoneIndex(BoneIdx);
        RefPoseComponentSpace[BoneIdx.GetInt()] =
            BoneContainer.GetRefPoseTransform(BoneIdx) * RefPoseComponentSpace[ParentIdx.GetInt()];
        Baked->RefPoseInverse[BoneIdx.GetInt()] =
            static_cast<FTransform3f>(RefPoseComponentSpace[BoneIdx.GetInt()]).Inverse().ToMatrixWithScale();
    }

    // ---- 4. Frame layout: frame 0 reserved for the reference pose, sequences occupy contiguous ranges after it ----
    const int32 SampleFrequency = FMath::Max(1, _SampleFrequency);
    int32 FrameCounter = 1; // frame 0 == reference pose
    Baked->Sequences.Reserve(_Sequences.Num());
    for (int32 SeqIndex = 0; SeqIndex < _Sequences.Num(); ++SeqIndex)
    {
        FCk_Iskm_BakedSequence BakedSeq;
        UAnimSequenceBase* const Seq = _Sequences[SeqIndex].Get_Sequence().Get();
        BakedSeq.Sequence = Seq;
        BakedSeq.SampleFrequency = SampleFrequency;
        BakedSeq.AnimationFrameCount = ck::IsValid(Seq)
            ? FMath::TruncToInt32(SampleFrequency * Seq->GetPlayLength()) + 1
            : 0;
        BakedSeq.AnimationFrameIndex = FrameCounter;
        FrameCounter += BakedSeq.AnimationFrameCount;
        Baked->Sequences.Add(BakedSeq);
    }
    Baked->FrameCountSequences = FrameCounter;
    Baked->TotalFrameCount = FrameCounter; // MVP: no transition / dynamic-pose region
    Baked->Matrices.SetNumUninitialized(RenderBoneCount * Baked->TotalFrameCount);

    // Per-frame culling bounds (Phase 4). MVP: the mesh's static bound, identical for every frame.
    const FBox3f MeshBound = static_cast<FBox3f>(_DefaultMesh->GetBounds().GetBox());
    Baked->FrameBounds.Init(MeshBound, Baked->FrameCountSequences);

    // ShaderMatrix[bone] = RefPoseInverse[bone] * ComponentSpaceBoneMatrix[bone], stored transposed 3x4.
    auto CalcRenderMatrices = [&Baked, RenderBoneCount](TArrayView<const FTransform> InPoseComponentSpace, int32 InFrameIndex)
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

    // ---- 5. Frame 0 = reference pose (yields identity matrices: RefPoseInverse * RefPoseComponentSpace) ----
    CalcRenderMatrices(RefPoseComponentSpace, 0);

    // ---- 6. Bake every sequence frame ----
    TArray<FTransform> PoseComponentSpace;
    PoseComponentSpace.SetNumUninitialized(BoneContainer.GetCompactPoseNumBones());

    for (int32 SeqIndex = 0; SeqIndex < _Sequences.Num(); ++SeqIndex)
    {
        UAnimSequenceBase* const Seq = _Sequences[SeqIndex].Get_Sequence().Get();
        if (ck::Is_NOT_Valid(Seq))
        { continue; }

#if WITH_EDITOR
        if (UAnimSequence* const AsAnimSeq = Cast<UAnimSequence>(Seq))
        { AsAnimSeq->WaitOnExistingCompression(true); }
#endif

        const FCk_Iskm_BakedSequence& BakedSeq = Baked->Sequences[SeqIndex];
        const double FrameTime = 1.0 / SampleFrequency;

        FCompactPose CompactPose;
        CompactPose.SetBoneContainer(&BoneContainer);
        FBlendedCurve Curve;
        Curve.InitFrom(BoneContainer);
        UE::Anim::FStackAttributeContainer Attributes;
        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);

        for (int32 LocalFrame = 0; LocalFrame < BakedSeq.AnimationFrameCount; ++LocalFrame)
        {
            const double SampleTime = LocalFrame * FrameTime;
            const int32 GlobalFrame = BakedSeq.AnimationFrameIndex + LocalFrame;

            CompactPose.ResetToRefPose();
            Seq->GetAnimationPose(PoseData, FAnimExtractContext(SampleTime, _bExtractRootMotion));

            // local (parent-relative) -> component space
            PoseComponentSpace[0] = CompactPose[FCompactPoseBoneIndex(0)];
            for (FCompactPoseBoneIndex BoneIdx(1); BoneIdx < CompactPose.GetNumBones(); ++BoneIdx)
            {
                const FCompactPoseBoneIndex ParentIdx = CompactPose.GetParentBoneIndex(BoneIdx);
                PoseComponentSpace[BoneIdx.GetInt()] = CompactPose[BoneIdx] * PoseComponentSpace[ParentIdx.GetInt()];
            }

            CalcRenderMatrices(PoseComponentSpace, GlobalFrame);
        }
    }

    Baked->bIsBaked = true;
    _BakedPose = MoveTemp(Baked);
    return true;
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
    return _BakedPose.IsValid() && _BakedPose->bIsBaked;
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
