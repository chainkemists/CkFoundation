#pragma once

#include "CkAnimationToolbox_Common.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

/** Owns the detection & mutation algorithms. Pure logic — no UI. */
class CKANIMATIONEDITOR_API FCkAnimScrunchDetector
{
public:
    using FProgressCallback = TFunction<bool(int32 Index, int32 Total, const FString& CurrentAssetName)>;

public:
    /**
     * Scans animation assets per Options and returns an info record for each.
     * The callback is invoked per-asset with progress; return false to cancel.
     * Returned array is sorted Affected-first.
     */
    auto Request_Scan(
        const FCk_AnimationToolbox_ScanOptions& Options,
        const FProgressCallback& OnProgress) const
        -> TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>;

    /**
     * Applies the chosen rewrite strategy to the given asset's frame 0.
     * Checks out via source control, mutates via IAnimationDataController, marks dirty.
     * Does NOT save the package — caller (or user) saves explicitly.
     */
    auto Request_ApplyFix(
        const TSharedPtr<FCk_AnimationToolbox_AnimInfo>& Info,
        ECk_AnimationToolbox_RewriteStrategy Strategy) const
        -> bool;

private:
    auto DoCollectCandidates(const FCk_AnimationToolbox_ScanOptions& Options) const -> TArray<FAssetData>;
    auto DoEvaluateAsset(UAnimSequence* Anim, float OutlierMultiplier, bool bRequireLoopBoundaryMismatch, FCk_AnimationToolbox_AnimInfo& OutInfo) const -> void;
    /** Fills OutTransforms[b] with the component-space transform of skeleton bone b at the requested frame. */
    auto DoBuildComponentSpacePose(UAnimSequence* Anim, int32 FrameIndex, const TSet<FName>& TrackNameSet, TArray<FTransform>& OutTransforms) const -> void;
    /** Summed per-bone (translation cm + rotation deg) between two component-space poses. */
    auto DoComputePoseDelta(const TArray<FTransform>& A, const TArray<FTransform>& B) const -> float;
    auto DoGatherUnmappedBones(UAnimSequence* Anim, FCk_AnimationToolbox_AnimInfo& OutInfo) const -> void;
};

// --------------------------------------------------------------------------------------------------------------------
