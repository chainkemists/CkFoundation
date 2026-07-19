#include "CkAnimBake.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimCurveFilter.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/MemStack.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_anim_bake
{
    // Full-skeleton bone container (compact-pose-bone-index == skeleton-bone-index), configured identically for
    // the reference-pose computation and the sampling pass — retargeting/raw-data flags affect both.
    auto
    MakeFullSkeletonBoneContainer(
        USkeleton& InSkeleton,
        const FCk_AnimBake_SampleParams& InParams,
        FBoneContainer& OutBoneContainer)
        -> void
    {
        const auto NumSkelBones = InSkeleton.GetReferenceSkeleton().GetNum();

        TArray<FBoneIndexType> AnimationRequiredBones;
        AnimationRequiredBones.SetNumUninitialized(NumSkelBones);
        for (int32 i = 0; i < NumSkelBones; ++i)
        { AnimationRequiredBones[i] = static_cast<FBoneIndexType>(i); }

        const UE::Anim::FCurveFilterSettings CurveFilter(UE::Anim::ECurveFilterMode::DisallowFiltered);
        OutBoneContainer.InitializeTo(MoveTemp(AnimationRequiredBones), CurveFilter, InSkeleton);
        OutBoneContainer.SetDisableRetargeting(InParams.DisableRetargeting);
        // Force RAW data in editor: at PostLoad some sequences may not have finished compression.
        OutBoneContainer.SetUseRAWData(static_cast<bool>(WITH_EDITOR));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::anim_bake::
    BuildSkeletonData(
        USkeleton& InSkeleton,
        const USkeletalMesh& InMesh,
        const FCk_AnimBake_SampleParams& InParams)
    -> TOptional<FCk_AnimBake_SkeletonData>
{
    // ---- Render-bone compaction: the skinned subset of skeleton bones (union over the mesh's LOD ActiveBoneIndices) ----
    const FReferenceSkeleton& SkelRefSkeleton = InSkeleton.GetReferenceSkeleton();
    const int32 NumSkelBones = SkelRefSkeleton.GetNum();
    if (NumSkelBones == 0)
    { return {}; }

    CK_ENSURE_IF_NOT(NumSkelBones <= static_cast<int32>(MAX_uint16) + 1,
        TEXT("Skeleton [{}] has [{}] bones — exceeds the uint16 render-bone index space"),
        InSkeleton.GetName(), NumSkelBones)
    { return {}; }

    const FSkeletalMeshRenderData* RenderData = InMesh.GetResourceForRendering();
    if (ck::Is_NOT_Valid(RenderData, ck::IsValid_Policy_NullptrOnly{}) || RenderData->LODRenderData.Num() == 0)
    { return {}; }

    TArray<bool> RequiredBones;
    RequiredBones.Init(false, NumSkelBones);
    for (int32 LODIndex = 0; LODIndex < RenderData->LODRenderData.Num(); ++LODIndex)
    {
        const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LODIndex];
        for (const FBoneIndexType MeshBoneIndex : LODRenderData.ActiveBoneIndices)
        {
            const int32 SkelBoneIndex = InSkeleton.GetSkeletonBoneIndexFromMeshBoneIndex(&InMesh, MeshBoneIndex);
            if (SkelBoneIndex != INDEX_NONE)
            { RequiredBones[SkelBoneIndex] = true; }
        }
    }

    FCk_AnimBake_SkeletonData SkeletonData;
    SkeletonData.SkeletonBoneToRenderBone.Init(INDEX_NONE, NumSkelBones);
    SkeletonData.RenderRequiredBones.Reserve(NumSkelBones);
    for (int32 SkelBoneIndex = 0; SkelBoneIndex < NumSkelBones; ++SkelBoneIndex)
    {
        if (RequiredBones[SkelBoneIndex])
        {
            SkeletonData.SkeletonBoneToRenderBone[SkelBoneIndex] = SkeletonData.RenderRequiredBones.Num();
            SkeletonData.RenderRequiredBones.Add(static_cast<uint16>(SkelBoneIndex));
        }
    }
    SkeletonData.RenderBoneCount = SkeletonData.RenderRequiredBones.Num();
    if (SkeletonData.RenderBoneCount == 0)
    { return {}; }

    // ---- Component-space reference pose + its inverse (per skeleton bone) ----
    FBoneContainer BoneContainer;
    ck_anim_bake::MakeFullSkeletonBoneContainer(InSkeleton, InParams, BoneContainer);
    const int32 NumContainerBones = BoneContainer.GetNumBones();

    // Local ref transform per (full-skeleton) bone. With UseMeshBindRefPose, invert the mesh bind pose (by bone
    // name, skeleton fallback) instead of the skeleton ref pose — see FCk_AnimBake_SampleParams::UseMeshBindRefPose.
    // The full-skeleton container guarantees compact-pose-bone-index == skeleton-bone-index.
    const FReferenceSkeleton& MeshRefSkel = InMesh.GetRefSkeleton();
    const FReferenceSkeleton& SkelRefSkel = InSkeleton.GetReferenceSkeleton();
    const auto Get_RefLocal = [&](FCompactPoseBoneIndex InBoneIdx) -> FTransform
    {
        if (InParams.UseMeshBindRefPose)
        {
            const int32 MeshBone = MeshRefSkel.FindBoneIndex(SkelRefSkel.GetBoneName(InBoneIdx.GetInt()));
            if (MeshBone != INDEX_NONE)
            { return MeshRefSkel.GetRefBonePose()[MeshBone]; }
        }
        return BoneContainer.GetRefPoseTransform(InBoneIdx);
    };

    SkeletonData.RefPoseComponentSpace.SetNumUninitialized(NumContainerBones);
    SkeletonData.RefPoseInverse.SetNumUninitialized(NumContainerBones);
    SkeletonData.RefPoseComponentSpace[0] = Get_RefLocal(FCompactPoseBoneIndex(0));
    SkeletonData.RefPoseInverse[0] =
        static_cast<FTransform3f>(SkeletonData.RefPoseComponentSpace[0]).Inverse().ToMatrixWithScale();
    for (FCompactPoseBoneIndex BoneIdx(1); BoneIdx < NumContainerBones; ++BoneIdx)
    {
        const FCompactPoseBoneIndex ParentIdx = BoneContainer.GetParentBoneIndex(BoneIdx);
        SkeletonData.RefPoseComponentSpace[BoneIdx.GetInt()] =
            Get_RefLocal(BoneIdx) * SkeletonData.RefPoseComponentSpace[ParentIdx.GetInt()];
        SkeletonData.RefPoseInverse[BoneIdx.GetInt()] =
            static_cast<FTransform3f>(SkeletonData.RefPoseComponentSpace[BoneIdx.GetInt()]).Inverse().ToMatrixWithScale();
    }

    return SkeletonData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::anim_bake::
    BuildFrameLayout(
        TArrayView<UAnimSequenceBase* const> InSequences,
        int32 InSampleFrequency)
    -> FCk_AnimBake_FrameLayout
{
    const int32 SampleFrequency = FMath::Max(1, InSampleFrequency);

    FCk_AnimBake_FrameLayout Layout;
    Layout.Sequences.Reserve(InSequences.Num());

    int32 FrameCounter = 1; // frame 0 == reference pose
    for (UAnimSequenceBase* const Seq : InSequences)
    {
        FCk_AnimBake_SequenceLayout SeqLayout;
        SeqLayout.Sequence = Seq;
        SeqLayout.SampleFrequency = SampleFrequency;
        SeqLayout.FrameCount = ck::IsValid(Seq)
            ? FMath::TruncToInt32(SampleFrequency * Seq->GetPlayLength()) + 1
            : 0;
        SeqLayout.FrameIndex = FrameCounter;
        FrameCounter += SeqLayout.FrameCount;
        Layout.Sequences.Add(SeqLayout);
    }
    Layout.TotalFrameCount = FrameCounter;

    return Layout;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::anim_bake::
    SamplePoses(
        USkeleton& InSkeleton,
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FCk_AnimBake_FrameLayout& InLayout,
        const FCk_AnimBake_SampleParams& InParams,
        const TFunctionRef<void(TArrayView<const FTransform> InPoseComponentSpace, int32 InGlobalFrame)>& InPerFramePose)
    -> FBox
{
    // Union of component-space render-bone positions across all sampled frames — feeds ComputeAnimatedBounds.
    FBox BoneBoundsAllFrames(ForceInit);

    CK_ENSURE_IF_NOT(InSkeletonData.RefPoseComponentSpace.Num() > 0,
        TEXT("Empty FCk_AnimBake_SkeletonData passed to SamplePoses for Skeleton [{}]"),
        InSkeleton.GetName())
    { return BoneBoundsAllFrames; }

    const auto AccumulateBoneBounds = [&BoneBoundsAllFrames, &InSkeletonData](TArrayView<const FTransform> InPoseComponentSpace)
    {
        for (int32 i = 0; i < InSkeletonData.RenderBoneCount; ++i)
        { BoneBoundsAllFrames += InPoseComponentSpace[InSkeletonData.RenderRequiredBones[i]].GetTranslation(); }
    };

    // ---- Frame 0 = reference pose ----
    AccumulateBoneBounds(InSkeletonData.RefPoseComponentSpace);
    InPerFramePose(InSkeletonData.RefPoseComponentSpace, 0);

    // ---- Every sequence frame ----
    FBoneContainer BoneContainer;
    ck_anim_bake::MakeFullSkeletonBoneContainer(InSkeleton, InParams, BoneContainer);

    // One hoisted consistency check bounds every RenderRequiredBones index against both pose arrays
    // (entries are < RefPoseComponentSpace.Num() by construction in BuildSkeletonData).
    CK_ENSURE_IF_NOT(BoneContainer.GetCompactPoseNumBones() == InSkeletonData.RefPoseComponentSpace.Num(),
        TEXT("Skeleton [{}] does not match the FCk_AnimBake_SkeletonData it is being sampled with (container bones [{}] vs skeleton-data bones [{}])"),
        InSkeleton.GetName(), BoneContainer.GetCompactPoseNumBones(), InSkeletonData.RefPoseComponentSpace.Num())
    { return BoneBoundsAllFrames; }

    TArray<FTransform> PoseComponentSpace;
    PoseComponentSpace.SetNumUninitialized(BoneContainer.GetCompactPoseNumBones());

    for (const FCk_AnimBake_SequenceLayout& SeqLayout : InLayout.Sequences)
    {
        if (SeqLayout.FrameCount == 0)
        { continue; } // sequence was already invalid when the layout was built — the slot is empty by design

        UAnimSequenceBase* const Seq = SeqLayout.Sequence.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(Seq),
            TEXT("Anim sequence for baked frame range [{} .. {}] was destroyed between BuildFrameLayout and SamplePoses — those frames will be left unwritten"),
            SeqLayout.FrameIndex, SeqLayout.FrameIndex + SeqLayout.FrameCount - 1)
        { continue; }

#if WITH_EDITOR
        if (UAnimSequence* const AsAnimSeq = Cast<UAnimSequence>(Seq))
        { AsAnimSeq->WaitOnExistingCompression(true); }
#endif

        const double FrameTime = 1.0 / SeqLayout.SampleFrequency;

        // FCompactPose/FStackAttributeContainer allocate on the mem stack — pose evaluation asserts
        // (MemStack.h NumMarks > 0) without an enclosing mark. Engine-tick callers get one for free;
        // standalone callers (bakers, automation tests) do not, so the core owns its own.
        FMemMark Mark(FMemStack::Get());

        FCompactPose CompactPose;
        CompactPose.SetBoneContainer(&BoneContainer);
        FBlendedCurve Curve;
        Curve.InitFrom(BoneContainer);
        UE::Anim::FStackAttributeContainer Attributes;
        FAnimationPoseData PoseData(CompactPose, Curve, Attributes);

        for (int32 LocalFrame = 0; LocalFrame < SeqLayout.FrameCount; ++LocalFrame)
        {
            const double SampleTime = LocalFrame * FrameTime;
            const int32 GlobalFrame = SeqLayout.FrameIndex + LocalFrame;

            CompactPose.ResetToRefPose();
            Seq->GetAnimationPose(PoseData, FAnimExtractContext(SampleTime, InParams.ExtractRootMotion));

            // local (parent-relative) -> component space
            PoseComponentSpace[0] = CompactPose[FCompactPoseBoneIndex(0)];
            for (FCompactPoseBoneIndex BoneIdx(1); BoneIdx < CompactPose.GetNumBones(); ++BoneIdx)
            {
                const FCompactPoseBoneIndex ParentIdx = CompactPose.GetParentBoneIndex(BoneIdx);
                PoseComponentSpace[BoneIdx.GetInt()] = CompactPose[BoneIdx] * PoseComponentSpace[ParentIdx.GetInt()];
            }

            AccumulateBoneBounds(PoseComponentSpace);
            InPerFramePose(PoseComponentSpace, GlobalFrame);
        }
    }

    return BoneBoundsAllFrames;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::anim_bake::
    ComputeAnimatedBounds(
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FBox& InBoneBoundsAllFrames,
        const USkeletalMesh& InMesh)
    -> FBox
{
    // Ref-pose skin pad: how far the mesh box extends beyond the ref-pose BONES — a measure of skin/cloth
    // offset from the skeleton, without touching vertex data.
    FBox RefBoneBounds(ForceInit);
    for (int32 i = 0; i < InSkeletonData.RenderBoneCount; ++i)
    { RefBoneBounds += InSkeletonData.RefPoseComponentSpace[InSkeletonData.RenderRequiredBones[i]].GetTranslation(); }

    const FBox MeshBox = InMesh.GetBounds().GetBox();
    const FVector SkinPad = FVector::Max(
        FVector::Max(RefBoneBounds.Min - MeshBox.Min, FVector::ZeroVector),
        FVector::Max(MeshBox.Max - RefBoneBounds.Max, FVector::ZeroVector));
    return InBoneBoundsAllFrames.ExpandBy(SkinPad) + MeshBox;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::anim_bake::
    Get_LoopedLocalFrame(
        float InTimeSeconds,
        int32 InSampleFrequency,
        int32 InFrameCount)
    -> int32
{
    if (InFrameCount <= 0)
    { return 0; }
    const int32 LocalFrame = FMath::TruncToInt32(InTimeSeconds * static_cast<float>(InSampleFrequency));
    return ((LocalFrame % InFrameCount) + InFrameCount) % InFrameCount;
}
