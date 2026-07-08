#include "CkIskmAnimCollection_Utils.h"
#include "CkIskmAnimCollection_BakedPose.h"

#include "CkCore/Validation/CkIsValid.h"

auto
    UCk_Utils_IskmAnimCollection_UE::
    Find_SequenceIndex_ByAsset(
        const UCk_IskmAnimCollection_Data* InAsset,
        const UAnimSequenceBase* InSequence)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return INDEX_NONE; }
    return InAsset->Find_SequenceIndex_ByAsset(InSequence);
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_Skeleton(const UCk_IskmAnimCollection_Data* InAsset)
    -> USkeleton*
{
    if (ck::Is_NOT_Valid(InAsset))
    { return nullptr; }
    // Effective skeleton: falls back to the DefaultMesh's skeleton when _Skeleton is unset
    // (script-authored collections can't assign it) — callers want "the skeleton this
    // collection animates with", which is exactly that.
    return InAsset->Get_EffectiveSkeleton();
}

// ---- Plan-2 CPU bake ----

auto
    UCk_Utils_IskmAnimCollection_UE::
    Build_BakedPoseData(UCk_IskmAnimCollection_Data* InAsset)
    -> bool
{
    if (ck::Is_NOT_Valid(InAsset))
    { return false; }
    return InAsset->Build_BakedPoseData();
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_IsBaked(const UCk_IskmAnimCollection_Data* InAsset)
    -> bool
{
    if (ck::Is_NOT_Valid(InAsset))
    { return false; }
    return InAsset->Get_IsBaked();
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_RenderBoneCount(const UCk_IskmAnimCollection_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->RenderBoneCount : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_TotalFrameCount(const UCk_IskmAnimCollection_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->TotalFrameCount : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_FrameCountSequences(const UCk_IskmAnimCollection_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->FrameCountSequences : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_BakedMatrixCount(const UCk_IskmAnimCollection_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->Get_MatrixCount() : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_BakedSequenceCount(const UCk_IskmAnimCollection_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->Sequences.Num() : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_SequenceFrameIndex(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return INDEX_NONE; }
    const auto* Baked = InAsset->Get_BakedPose();
    if (Baked == nullptr || NOT Baked->Sequences.IsValidIndex(InSequenceIndex))
    { return INDEX_NONE; }
    return Baked->Sequences[InSequenceIndex].AnimationFrameIndex;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_SequenceFrameCount(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    if (Baked == nullptr || NOT Baked->Sequences.IsValidIndex(InSequenceIndex))
    { return 0; }
    return Baked->Sequences[InSequenceIndex].AnimationFrameCount;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_GlobalFrame(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex, int32 InLocalFrame)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->Get_GlobalFrame(InSequenceIndex, InLocalFrame) : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_IsRefPoseFrameIdentity(const UCk_IskmAnimCollection_Data* InAsset, float InTolerance)
    -> bool
{
    if (ck::Is_NOT_Valid(InAsset))
    { return false; }
    const auto* Baked = InAsset->Get_BakedPose();
    if (Baked == nullptr || Baked->RenderBoneCount == 0 || Baked->Matrices.Num() < Baked->RenderBoneCount)
    { return false; }

    // The transposed 3x4 of the identity matrix.
    const float Identity[12] = { 1.f, 0.f, 0.f, 0.f,
                                 0.f, 1.f, 0.f, 0.f,
                                 0.f, 0.f, 1.f, 0.f };

    for (int32 BoneIdx = 0; BoneIdx < Baked->RenderBoneCount; ++BoneIdx)
    {
        const FCk_Iskm_BoneMatrix3x4& Mtx = Baked->Matrices[BoneIdx]; // frame 0 = first RenderBoneCount entries
        for (int32 e = 0; e < 12; ++e)
        {
            if (FMath::Abs(Mtx.M[e] - Identity[e]) > InTolerance)
            { return false; }
        }
    }
    return true;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_LoopedFrameAtTime(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex, float InTimeSeconds)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->Get_LoopedFrameAtTime(InSequenceIndex, InTimeSeconds) : 0;
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_SequenceSampleFrequency(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    const auto* Baked = InAsset->Get_BakedPose();
    return Baked != nullptr ? Baked->Get_SequenceSampleFrequency(InSequenceIndex) : 0;
}
