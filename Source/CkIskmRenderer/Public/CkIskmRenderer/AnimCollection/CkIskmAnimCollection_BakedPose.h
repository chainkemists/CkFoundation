#pragma once

#include "CoreMinimal.h"

#include "CkIskmRendererVF/CkIskm_BoneMatrix.h"

// --------------------------------------------------------------------------------------------------------------------
// Transposed 3x4 component-space pose per (frame x render-bone), GPU-upload-ready as a Buffer<float4> SRV:
//     ShaderMatrix[bone] = RefPoseInverse[bone] * ComponentSpaceBoneMatrix[bone]
//     matrixIndex = frameIndex * RenderBoneCount + boneIndex        (each entry = 3 x float4)
// Frame 0 is the reference pose (identity); sequences occupy contiguous frame ranges from frame 1 on.

class UAnimSequenceBase;

struct FCk_Iskm_BakedSequence
{
    // Base frame offset into the flat matrix buffer for this sequence's frame 0.
    int32 AnimationFrameIndex = 0;
    // Number of sampled frames = Trunc(SampleFrequency * PlayLength) + 1.
    int32 AnimationFrameCount = 0;
    int32 SampleFrequency = 30;
    TWeakObjectPtr<UAnimSequenceBase> Sequence;
};

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

struct FCk_Iskm_BakedPose
{
    // Number of bones written per frame (the skinned subset of skeleton bones).
    int32 RenderBoneCount = 0;
    // 1 (identity frame 0) + sum of per-sequence frame counts.
    int32 FrameCountSequences = 0;
    int32 TotalFrameCount = 0;
    bool HighPrecision = true;

    // [TotalFrameCount * RenderBoneCount], index = frame * RenderBoneCount + bone.
    TArray<FCk_Iskm_BoneMatrix3x4> Matrices;
    // [FrameCountSequences] per-frame local-space AABB for culling.
    TArray<FBox3f> FrameBounds;

    // Bone-position union across every baked frame + ref-pose skin pad, unioned with the mesh box: an
    // animated pose (arm swings, jumps) can exceed the static mesh box, which clips silhouettes when culled.
    FBox AnimatedBounds = FBox(ForceInit);

    // render-bone index -> skeleton-bone index.
    TArray<uint16> RenderRequiredBones;
    // skeleton-bone index -> render-bone index (INDEX_NONE if that bone is unskinned / not rendered).
    TArray<int32> SkeletonBoneToRenderBone;
    // per skeleton bone: inverse of the component-space reference pose matrix.
    TArray<FMatrix44f> RefPoseInverse;
    // per-sequence offset table, parallel to the asset's _Sequences array.
    TArray<FCk_Iskm_BakedSequence> Sequences;

    // one entry per RESOLVABLE _BakedSockets name (unresolvable ones are skipped at bake).
    TArray<FCk_Iskm_BakedSocket> Sockets;

    bool IsBaked = false;

    FORCEINLINE int32 Get_MatrixCount() const { return Matrices.Num(); }

    // Resolve a sequence-local frame to a global buffer frame index (clamped to the sequence range).
    auto
    Get_GlobalFrame(int32 InSequenceIndex, int32 InLocalFrame) const -> int32;

    // LOOPING advance: GlobalFrame = AnimationFrameIndex + (trunc(time * SampleFrequency) mod AnimationFrameCount),
    // negative time wrapped. The batched sync uploads the result as the per-instance frame index.
    auto
    Get_LoopedFrameAtTime(int32 InSequenceIndex, float InTimeSeconds) const -> int32;

    auto
    Get_SequenceSampleFrequency(int32 InSequenceIndex) const -> int32;

    auto
    Find_Socket(FName InName) const -> const FCk_Iskm_BakedSocket*;
};
