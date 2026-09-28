#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProceduralChainAvoidanceSolver;

    // A caller-owned, already world/body-admitted pose. Its joints remain live throughout SelectProceduralChainPoses.
    struct CKPROCEDURALANIMATION_API FProceduralChainPoseCandidate
    {
    public:
        CK_GENERATED_BODY(FProceduralChainPoseCandidate);

    private:
        TArrayView<const FVector> _Joints;

    public:
        CK_PROPERTY(_Joints);
        CK_DEFINE_CONSTRUCTORS(FProceduralChainPoseCandidate, _Joints);
    };

    // Radii has one finite nonnegative world-centimetre value per link. All candidates have the same link count.
    // StableId is unique within the body; it determines traversal independently of input-array order.
    struct CKPROCEDURALANIMATION_API FProceduralChainAvoidanceLeg
    {
    public:
        CK_GENERATED_BODY(FProceduralChainAvoidanceLeg);

    private:
        uint32 _StableId = 0;
        TArrayView<const FProceduralChainPoseCandidate> _Candidates;
        TArrayView<const float> _Radii;
        int32 _InitialCandidate = INDEX_NONE;

    public:
        CK_PROPERTY(_StableId);
        CK_PROPERTY(_Candidates);
        CK_PROPERTY(_Radii);
        CK_PROPERTY(_InitialCandidate);
        CK_DEFINE_CONSTRUCTORS(FProceduralChainAvoidanceLeg, _StableId, _Candidates, _Radii, _InitialCandidate);
    };

    struct CKPROCEDURALANIMATION_API FProceduralChainAvoidanceOutcome
    {
    public:
        CK_GENERATED_BODY(FProceduralChainAvoidanceOutcome);

    private:
        double _MaxPenetration = 0.0;
        double _TotalPenalty = 0.0;
        int32 _CandidateEvaluations = 0;
        int32 _Passes = 0;

    public:
        CK_PROPERTY(_MaxPenetration);
        CK_PROPERTY(_TotalPenalty);
        CK_PROPERTY(_CandidateEvaluations);
        CK_PROPERTY(_Passes);
    };

    // Reuse per body to retain allocation capacity. Contains no handles, queries or ownership of input joint arrays.
    struct CKPROCEDURALANIMATION_API FProceduralChainAvoidanceScratch
    {
    public:
        CK_GENERATED_BODY(FProceduralChainAvoidanceScratch);
        friend class FProceduralChainAvoidanceSolver;

    private:
        TArray<FBox> _Bounds;
        TArray<int32> _CandidateOffsets;
        TArray<int32> _Choices;
        TArray<int32> _Order;
        TArray<double> _PairPenalties;
        TArray<double> _CandidateRow;
        TArray<double> _BestRow;
        TArray<uint32> _CrossingMasks;
    };

    // Select from at most 64 legs, 24 poses per leg and 8 links per pose. Three stable-order coordinate-descent passes
    // minimize the sum of squared sibling capsule penetrations; only strict improvement changes a pick. Equal scores
    // retain the current pose. This bounded search is not a global optimum and may leave unavoidable or locally trapped
    // overlap. No supplied pose is altered, so the caller's admission constraints remain authoritative.
    // Outputs have one entry per input leg and may not overlap in memory. CrossingLinks counts distinct links overlapping
    // any sibling capsule, not pair multiplicity. Invalid/arithmetically unrepresentable geometry rejects atomically:
    // choices, crossing counts and outcome are unchanged; scratch may be reused on the next call.
    CKPROCEDURALANIMATION_API auto
        SelectProceduralChainPoses(
            TArrayView<const FProceduralChainAvoidanceLeg> InLegs,
            FProceduralChainAvoidanceScratch& InOutScratch,
            TArrayView<int32> OutChoices,
            TArrayView<int32> OutCrossingLinks,
            FProceduralChainAvoidanceOutcome& OutOutcome)
        -> bool;

    // --------------------------------------------------------------------------------------------------------------------

    // The wide tiers come last, so they are tried only when every narrower angle crosses.
    inline constexpr float ProceduralPoleSwivelFanDegrees[] = {0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f, 120.0f, -120.0f,
        150.0f, -150.0f};

    // InPole rotated about the axis from InHip through InFoot by InDegrees (right-handed about hip->foot). A degenerate
    // axis (hip on the foot) or non-finite input returns InPole unchanged.
    CKPROCEDURALANIMATION_API auto
        ComputeProceduralPoleSwivel(
            const FVector& InHip,
            const FVector& InFoot,
            const FVector& InPole,
            float InDegrees)
        -> FVector;

    // The fan order for one solve: InLastClearDegrees first when it is a fan angle other than 0, then 0, then the fan's
    // remaining angles in fan order. Writes at most the fan's count into OutOrder and returns the count.
    CKPROCEDURALANIMATION_API auto
        Get_ProceduralPoleSwivelOrder(
            float InLastClearDegrees,
            TArrayView<float> OutOrder)
        -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
