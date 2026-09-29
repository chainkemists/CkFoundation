#pragma once

#include "CkChain/CkChain_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::chain
{
    struct CKCHAIN_API FPathHistory
    {
    public:
        /** Replaces the logical history with two seed samples while retaining allocated capacity. */
        auto Reseed(const FTransform& InHeadPose, float InSpacingCm) -> void;
        /** Requires seeded history; appends at most one sample and overwrites the oldest when full. */
        auto Record(const FTransform& InHeadPose, float InSpacingCm) -> bool;
        /** Grows capacity for the distance window without changing logical sample order. */
        auto Reserve_ForDistance(float InDistanceCm, float InSpacingCm) -> void;
        /** Retains the predecessor of the cutoff and at least two samples when available. */
        auto Trim(float InKeepBehindHeadCm) -> void;
        /** Empty history returns unset; HoldUntilCovered also returns unset behind the oldest sample. */
        auto Sample_AtArcDistance(float InS, ECk_Chain_HistorySeed InSeed) const -> TOptional<FCk_Chain_PathSample>;
        /** Histories with fewer than two samples have no tangent and return zero. */
        auto Tangent_AtArcDistance(float InS) const -> FVector;
        /** Copies exactly the samples at or behind the cutoff, rebasing the newest copied sample to zero. */
        auto Slice_Behind(float InMaxS) const -> FPathHistory;
        /** Requires at least one sample. */
        auto Get_HeadArcDistance() const -> float;
        /** Requires at least one sample. */
        auto Get_OldestArcDistance() const -> float;
        auto Get_NumSamples() const -> int32;
        /** InIndex must lie in [0, Get_NumSamples()). References expire on reserve or overwrite. */
        auto Get_Sample(int32 InIndex) const -> const FCk_Chain_PathSample&;
        auto Get_Samples() const -> TArray<FCk_Chain_PathSample>;
        auto Get_AllocatedSize() const -> SIZE_T;

    private:
        auto DoGet_BracketStart(float InS) const -> int32;
        auto DoAppend(const FCk_Chain_PathSample& InSample) -> void;

        TArray<FCk_Chain_PathSample> _Buffer;
        int32 _Start = 0;
        int32 _Count = 0;
    };

    // Arc position of the head itself: the newest recorded sample's arc distance plus the chord the head has
    // travelled since that sample (below the spacing threshold, so not yet a sample of its own). Zero-sample
    // histories return 0.
    CKCHAIN_API auto Get_LeadingArcDistance(
        const FPathHistory& InHistory,
        const FTransform& InHeadPose) -> float;

    CKCHAIN_API auto Solve_PathHistoryPose(
        const FPathHistory& InHistory,
        const FTransform& InHeadPose,
        float InDistanceFromHeadCm,
        ECk_Chain_LinkOrientation InOrientation,
        const FVector& InUpNormalized,
        ECk_Chain_HistorySeed InSeed,
        const FTransform& InLinkCurrentPose) -> TOptional<FTransform>;

    /** Zero-length segments are placed at their predecessor and retain their input rotation. */
    CKCHAIN_API auto Solve_DistanceConstraint(
        const FTransform& InHeadPose,
        TArrayView<const float> InSegmentLengthsCm,
        TArrayView<const ECk_Chain_LinkOrientation> InOrientations,
        const FVector& InUpNormalized,
        TArrayView<FTransform> InOutLinkPoses) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
