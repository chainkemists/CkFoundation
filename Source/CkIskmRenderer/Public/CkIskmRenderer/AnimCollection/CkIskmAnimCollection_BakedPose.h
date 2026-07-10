#pragma once

#include "CoreMinimal.h"

// FCk_Iskm_BoneMatrix3x4 lives in the engine-only PostConfigInit VF module (shared by the baker + the GPU upload).
#include "CkIskmRendererVF/CkIskm_BoneMatrix.h"

// ====================================================================================================================
//  CkIskmRenderer Plan-2 — CPU bone-matrix bake output
//
//  The bake (UCk_IskmAnimCollection_Data::Build_BakedPoseData) samples every sequence at a fixed frequency and
//  stores, per (frame x render-bone), the transposed 3x4 component-space pose relative to the reference pose:
//
//      ShaderMatrix[bone] = RefPoseInverse[bone] * ComponentSpaceBoneMatrix[bone]   (stored transposed, 3 rows of float4)
//
//  Flat layout, GPU-upload-ready as a Buffer<float4> SRV (Phase 1):
//      matrixIndex = frameIndex * RenderBoneCount + boneIndex      (each entry = 3 x float4 = 12 floats)
//
//  Frame 0 is the reference pose (identity matrices). Sequences occupy contiguous frame ranges starting at frame 1.
//  This is a direct port of Skelot's FSkelotAnimationBuffer bake (SkelotAnimCollection.cpp CalcRenderMatrices).
//
//  This data is asset-intrinsic (world-independent), transient (rebuilt, never serialized), and CPU-only — the bake
//  touches no RHI, so it runs headlessly under -nullrhi (the SRV upload is a separate Phase-1 step).
// ====================================================================================================================

class UAnimSequenceBase;

// Per-sequence offset table entry (mirrors Skelot FSkelotSequenceDef's render-relevant fields).
struct FCk_Iskm_BakedSequence
{
    // Base frame offset into the flat matrix buffer for this sequence's frame 0.
    int32 AnimationFrameIndex = 0;
    // Number of sampled frames = Trunc(SampleFrequency * PlayLength) + 1.
    int32 AnimationFrameCount = 0;
    int32 SampleFrequency = 30;
    TWeakObjectPtr<UAnimSequenceBase> Sequence;
};

// Per-frame component-space transforms for one baked socket (inc-4 far cosmetics).
struct FCk_Iskm_BakedSocket
{
    FName Name;
    // Skeleton-bone index the socket rides (unresolvable entries are skipped at bake, never stored).
    int32 BoneIndex = INDEX_NONE;
    // Socket local offset relative to its bone (identity when Name resolved to a bare bone).
    FTransform3f LocalOffset = FTransform3f::Identity;
    // [TotalFrameCount] component-space socket transform per baked frame (frame 0 = reference pose).
    TArray<FTransform3f> FrameTransforms;
};

// Full CPU bake for one AnimCollection.
struct FCk_Iskm_BakedPose
{
    // Number of bones written per frame (the skinned subset of skeleton bones).
    int32 RenderBoneCount = 0;
    // 1 (identity frame 0) + sum of per-sequence frame counts.
    int32 FrameCountSequences = 0;
    // MVP: == FrameCountSequences (no transition / dynamic-pose region yet).
    int32 TotalFrameCount = 0;
    // MVP: always high-precision float32. (Skelot defaults to float16; HP is strictly higher quality.)
    bool HighPrecision = true;

    // [TotalFrameCount * RenderBoneCount], index = frame * RenderBoneCount + bone.
    TArray<FCk_Iskm_BoneMatrix3x4> Matrices;
    // [FrameCountSequences] per-frame local-space AABB for culling (Phase 4). MVP: mesh static bound per frame.
    TArray<FBox3f> FrameBounds;

    // Conservative ANIMATED bounds: union of component-space bone positions across every baked frame, expanded
    // by the ref-pose skin pad (how far the mesh box extends beyond the ref-pose bones), unioned with the mesh
    // box. Animated poses (walk/jog arm swings, jumps) can exceed the static mesh box — culling with the raw
    // mesh box clips silhouettes at bound edges.
    FBox AnimatedBounds = FBox(ForceInit);

    // render-bone index -> skeleton-bone index.
    TArray<uint16> RenderRequiredBones;
    // skeleton-bone index -> render-bone index (INDEX_NONE if that bone is unskinned / not rendered).
    TArray<int32> SkeletonBoneToRenderBone;
    // per skeleton bone: inverse of the component-space reference pose matrix.
    TArray<FMatrix44f> RefPoseInverse;
    // per-sequence offset table, parallel to the asset's _Sequences array.
    TArray<FCk_Iskm_BakedSequence> Sequences;

    // Baked socket table (inc-4 far cosmetics) — one entry per resolvable _BakedSockets name.
    TArray<FCk_Iskm_BakedSocket> Sockets;

    bool IsBaked = false;

    FORCEINLINE int32 Get_MatrixCount() const { return Matrices.Num(); }

    // Resolve a sequence-local frame to a global buffer frame index (clamped to the sequence range).
    auto
    Get_GlobalFrame(int32 InSequenceIndex, int32 InLocalFrame) const -> int32;

    // Per-instance frame advance for a LOOPING sequence: GlobalFrame = AnimationFrameIndex + (trunc(time*rate) mod count).
    // This is the CPU-side of Skelot's UpdateAnimation (LocalFrame = trunc(time*SampleFrequency)); the batched sync
    // uploads the result as the per-instance frame index. Negative wrap handled for safety.
    auto
    Get_LoopedFrameAtTime(int32 InSequenceIndex, float InTimeSeconds) const -> int32;

    auto
    Get_SequenceSampleFrequency(int32 InSequenceIndex) const -> int32;

    auto
    Find_Socket(FName InName) const -> const FCk_Iskm_BakedSocket*;
};
