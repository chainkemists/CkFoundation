#pragma once

#include "CoreMinimal.h"

// ====================================================================================================================
//  ck::anim_bake — shared CPU animation-sampling core for frame-baked renderers (CkIskmRenderer, CkVat).
//  Encoding-independent: consumers supply a per-frame callback that encodes the pose into their own target.
//  CPU-only, no RHI — safe headless under -nullrhi. Design + provenance: CkAnimation/Claude.md.
// ====================================================================================================================

class USkeleton;
class USkeletalMesh;
class UAnimSequenceBase;

// "Render bones" = the skinned subset of skeleton bones (union over the mesh's LOD ActiveBoneIndices).
struct CKANIMATION_API FCk_AnimBake_SkeletonData
{
    int32 RenderBoneCount = 0;
    // render-bone index -> skeleton-bone index.
    TArray<uint16> RenderRequiredBones;
    // skeleton-bone index -> render-bone index (INDEX_NONE if that bone is unskinned / not rendered).
    TArray<int32> SkeletonBoneToRenderBone;
    // both indexed by SKELETON bone.
    TArray<FTransform> RefPoseComponentSpace;
    TArray<FMatrix44f> RefPoseInverse;
};

struct CKANIMATION_API FCk_AnimBake_SequenceLayout
{
    // Global frame index of this sequence's local frame 0.
    int32 FrameIndex = 0;
    // Number of sampled frames = Trunc(SampleFrequency * PlayLength) + 1 (0 for invalid sequences).
    int32 FrameCount = 0;
    int32 SampleFrequency = 30;
    TWeakObjectPtr<UAnimSequenceBase> Sequence;
};

// Flat frame layout: frame 0 is reserved for the reference pose; sequences occupy contiguous ranges after it.
struct CKANIMATION_API FCk_AnimBake_FrameLayout
{
    // 1 (reference-pose frame 0) + sum of per-sequence frame counts.
    int32 TotalFrameCount = 0;
    TArray<FCk_AnimBake_SequenceLayout> Sequences;
};

// Sampling knobs shared by the reference-pose computation and the per-frame sampling (they must match —
// retargeting affects both).
struct CKANIMATION_API FCk_AnimBake_SampleParams
{
    bool ExtractRootMotion = false;
    bool DisableRetargeting = false;

    // Invert the DefaultMesh bind pose instead of the skeleton ref pose when building RefPoseInverse.
    // Off by default; the "strafing" failure it fixes is in CkAnimation/Claude.md.
    bool UseMeshBindRefPose = false;
};

namespace ck::anim_bake
{
    // Unset when the pair is not bakeable (no skeleton bones, no render data, or no skinned bones) —
    // callers decide loudness.
    CKANIMATION_API auto
    BuildSkeletonData(
        USkeleton& InSkeleton,
        const USkeletalMesh& InMesh,
        const FCk_AnimBake_SampleParams& InParams)
        -> TOptional<FCk_AnimBake_SkeletonData>;

    // Frame layout for a sequence list at one fixed sample frequency (clamped to >= 1).
    // Invalid sequences keep their slot with FrameCount 0, so layout indices stay parallel to the input.
    CKANIMATION_API auto
    BuildFrameLayout(
        TArrayView<UAnimSequenceBase* const> InSequences,
        int32 InSampleFrequency)
        -> FCk_AnimBake_FrameLayout;

    // Samples frame 0 (the reference pose) and every sequence frame at the layout's frequency, invoking
    // InPerFramePose with the component-space pose (indexed by SKELETON bone) and the global frame index.
    // Returns the union of the render-bone translations across every sampled frame (feeds animated bounds).
    CKANIMATION_API auto
    SamplePoses(
        USkeleton& InSkeleton,
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FCk_AnimBake_FrameLayout& InLayout,
        const FCk_AnimBake_SampleParams& InParams,
        const TFunctionRef<void(TArrayView<const FTransform> InPoseComponentSpace, int32 InGlobalFrame)>& InPerFramePose)
        -> FBox;

    // Conservative animated culling bounds: every baked frame's bone positions, expanded by the ref-pose skin
    // pad, unioned with the mesh box — never smaller than the static bound (arm swings and jumps exceed it).
    CKANIMATION_API auto
    ComputeAnimatedBounds(
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FBox& InBoneBoundsAllFrames,
        const USkeletalMesh& InMesh)
        -> FBox;

    // Per-instance frame advance for a LOOPING clip: LocalFrame = trunc(time * frequency) mod count,
    // negative wrap handled. Returns 0 when the clip has no frames.
    CKANIMATION_API auto
    Get_LoopedLocalFrame(
        float InTimeSeconds,
        int32 InSampleFrequency,
        int32 InFrameCount)
        -> int32;
}
