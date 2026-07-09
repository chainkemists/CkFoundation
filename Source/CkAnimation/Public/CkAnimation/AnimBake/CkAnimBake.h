#pragma once

#include "CoreMinimal.h"

// ====================================================================================================================
//  ck::anim_bake — shared CPU animation-sampling core for frame-baked renderers.
//
//  Extracted from CkIskmRenderer Plan-2's Build_BakedPoseData (itself a Skelot port): the parts of a bake that are
//  independent of the OUTPUT ENCODING — render-bone compaction, component-space reference pose (+ inverse), the
//  frame layout (frame 0 = reference pose, sequences in contiguous ranges), fixed-frequency pose sampling, and the
//  conservative animated bounds. Consumers supply a per-frame callback that encodes the pose into their own target:
//    - CkIskmRenderer: transposed 3x4 bone matrices, flat Buffer<float4>-ready array (bone-palette skinning).
//    - CkVat:          bone/vertex animation TEXTURES (VAT bake; vertex mode additionally CPU-skins per vertex).
//
//  CPU-only, no RHI — safe headless under -nullrhi. Editor-time and load-time callers both use this.
// ====================================================================================================================

class USkeleton;
class USkeletalMesh;
class UAnimSequenceBase;

// Render-bone compaction + component-space reference pose for one (skeleton, mesh) pair.
// "Render bones" = the skinned subset of skeleton bones (union over the mesh's LOD ActiveBoneIndices).
struct CKANIMATION_API FCk_AnimBake_SkeletonData
{
    int32 RenderBoneCount = 0;
    // render-bone index -> skeleton-bone index.
    TArray<uint16> RenderRequiredBones;
    // skeleton-bone index -> render-bone index (INDEX_NONE if that bone is unskinned / not rendered).
    TArray<int32> SkeletonBoneToRenderBone;
    // per skeleton bone: component-space reference pose.
    TArray<FTransform> RefPoseComponentSpace;
    // per skeleton bone: inverse of the component-space reference pose matrix.
    TArray<FMatrix44f> RefPoseInverse;
};

// One sequence's slot in the flat frame layout.
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

    // Invert the DefaultMesh bind pose instead of the skeleton ref pose when building RefPoseInverse (port of
    // Skelot's RefPoseOverrideMesh, defaulted to the mesh). The skeleton ref pose and the anims both carry the +X
    // import reorientation while the mesh binds facing -Y — inverting the skeleton pose cancels the reorientation,
    // so skinned output faces -Y while moving +X ("strafing"). Mesh-bind matches the engine SKMC contract
    // (GetRefBasesInvMatrix() * pose). Off by default (skeleton ref pose) so existing bakers are unaffected.
    bool UseMeshBindRefPose = false;
};

namespace ck::anim_bake
{
    // Render-bone compaction + component-space ref pose (+ inverse). Unset when the pair is not bakeable
    // (no skeleton bones, no render data, or no skinned bones) — callers decide loudness.
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
    // pad (how far the mesh box extends beyond the ref-pose bones), unioned with the mesh box — never smaller
    // than the static bound. Animated poses (arm swings, jumps) can exceed the mesh box; culling with the raw
    // box clips silhouettes at bound edges.
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
