#include "CkIskmAnimCollection_BakedPose.h"

#include "CkAnimation/AnimBake/CkAnimBake.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Iskm_BakedPose::
    Get_GlobalFrame(
        int32 InSequenceIndex,
        int32 InLocalFrame) const
    -> int32
{
    if (NOT Sequences.IsValidIndex(InSequenceIndex))
    { return 0; }

    const FCk_Iskm_BakedSequence& Seq = Sequences[InSequenceIndex];
    const int32 Local = FMath::Clamp(InLocalFrame, 0, FMath::Max(0, Seq.AnimationFrameCount - 1));
    return Seq.AnimationFrameIndex + Local;
}

auto
    FCk_Iskm_BakedPose::
    Get_LoopedFrameAtTime(
        int32 InSequenceIndex,
        float InTimeSeconds) const
    -> int32
{
    if (NOT Sequences.IsValidIndex(InSequenceIndex))
    { return 0; }

    // Looped time->frame math lives in the shared bake core (also drives the CkVat shader contract).
    const FCk_Iskm_BakedSequence& Seq = Sequences[InSequenceIndex];
    return Seq.AnimationFrameIndex +
        ck::anim_bake::Get_LoopedLocalFrame(InTimeSeconds, Seq.SampleFrequency, Seq.AnimationFrameCount);
}

auto
    FCk_Iskm_BakedPose::
    Get_SequenceSampleFrequency(
        int32 InSequenceIndex) const
    -> int32
{
    return Sequences.IsValidIndex(InSequenceIndex) ? Sequences[InSequenceIndex].SampleFrequency : 30;
}
