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
    bool bHighPrecision = true;

    // [TotalFrameCount * RenderBoneCount], index = frame * RenderBoneCount + bone.
    TArray<FCk_Iskm_BoneMatrix3x4> Matrices;
    // [FrameCountSequences] per-frame local-space AABB for culling (Phase 4). MVP: mesh static bound per frame.
    TArray<FBox3f> FrameBounds;

    // render-bone index -> skeleton-bone index.
    TArray<uint16> RenderRequiredBones;
    // skeleton-bone index -> render-bone index (INDEX_NONE if that bone is unskinned / not rendered).
    TArray<int32> SkeletonBoneToRenderBone;
    // per skeleton bone: inverse of the component-space reference pose matrix.
    TArray<FMatrix44f> RefPoseInverse;
    // per-sequence offset table, parallel to the asset's _Sequences array.
    TArray<FCk_Iskm_BakedSequence> Sequences;

    bool bIsBaked = false;

    FORCEINLINE int32 Get_MatrixCount() const { return Matrices.Num(); }

    // Resolve a sequence-local frame to a global buffer frame index (clamped to the sequence range).
    int32 Get_GlobalFrame(int32 InSequenceIndex, int32 InLocalFrame) const
    {
        if (!Sequences.IsValidIndex(InSequenceIndex)) { return 0; }
        const FCk_Iskm_BakedSequence& Seq = Sequences[InSequenceIndex];
        const int32 Local = FMath::Clamp(InLocalFrame, 0, FMath::Max(0, Seq.AnimationFrameCount - 1));
        return Seq.AnimationFrameIndex + Local;
    }
};
