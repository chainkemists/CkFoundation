#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkCrowdAgent_Diag_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_CrowdAgent_DiagSensorTap;
    class FProcessor_CrowdAgent_DiagSeparationTap;
    class FProcessor_CrowdAgent_DiagSteeringTap;
    class FProcessor_CrowdAgent_DiagAvoidanceTap;
    class FProcessor_CrowdAgent_DiagAvoidanceScoreTap;
    class FProcessor_CrowdAgent_DiagAccelTap;
    class FProcessor_CrowdAgent_DiagVelocityBridgeTap;
    class FProcessor_CrowdAgent_DiagApplyOffsetTap;
    class FProcessor_CrowdAgent_DiagPushApartTap;
    class FProcessor_CrowdAgent_DiagRecorder;
    class FProcessor_CrowdAgent_DiagDrawBreadcrumb;
}

class UCk_Utils_CrowdAgent_Diag_UE;

enum class ECk_CrowdDiag_SpatialWindowResetReason : uint8
{
    None,
    Reached,
    SpeedBelowMinimum,
    RadiusBelowMinimum,
};

// --------------------------------------------------------------------------------------------------------------------
struct FCk_CrowdDiag_AvoidanceScore final
{
    float _DesiredVelocityPenalty = 0.0f;
    float _CurrentVelocityPenalty = 0.0f;
    float _TimeToImpactPenalty = 0.0f;
    float _SidePenalty = 0.0f;
    float _TotalPenalty = 0.0f;
};

struct FCk_CrowdDiag_TimeToImpactContributor final
{
    bool _Observed = false;
    int32 _NeighborIndex = INDEX_NONE;
    uint32 _NeighborHandleHash = 0;
    float _Time = 0.0f;
    FVector _RelativeOffset = FVector::ZeroVector;
    FVector _RelativeVelocity = FVector::ZeroVector;
};

struct FCk_CrowdDiag_PriorWinnerRescore final
{
    bool _Observed = false;
    int32 _SourceSampledFrame = INDEX_NONE;
    int32 _AgeFrames = INDEX_NONE;
    FVector _RawVelocity = FVector::ZeroVector;
    FVector _ScoredVelocity = FVector::ZeroVector;
    float _HeadingDeltaToCurrentWinnerDeg = 0.0f;
    FCk_CrowdDiag_AvoidanceScore _Score;
    float _MinusCurrentWinnerPenalty = 0.0f;
    FCk_CrowdDiag_TimeToImpactContributor _TimeToImpactContributor;

    int32 _NearestFinalCandidateIndex = INDEX_NONE;
    FVector _NearestFinalCandidateRawVelocity = FVector::ZeroVector;
    FVector _NearestFinalCandidateScoredVelocity = FVector::ZeroVector;
    float _NearestFinalCandidateDistance = 0.0f;
    FCk_CrowdDiag_AvoidanceScore _NearestFinalCandidateScore;
    float _NearestFinalCandidateMinusWinnerPenalty = 0.0f;
    FCk_CrowdDiag_TimeToImpactContributor _NearestFinalCandidateTimeToImpactContributor;
};

// --------------------------------------------------------------------------------------------------------------------
struct FCk_CrowdDiag_AvoidanceDepthResult final
{
    int32 _Depth = 0;
    FVector _Centre = FVector::ZeroVector;
    float _Radius = 0.0f;
    int32 _SelectedCandidateIndex = INDEX_NONE;
    FVector _SelectedCandidateVelocity = FVector::ZeroVector;
    FVector _SelectedScoredVelocity = FVector::ZeroVector;
    float _SelectedDesiredVelocityPenalty = 0.0f;
    float _SelectedCurrentVelocityPenalty = 0.0f;
    float _SelectedTimeToImpactPenalty = 0.0f;
    float _SelectedSidePenalty = 0.0f;
    float _SelectedTotalPenalty = 0.0f;
    bool _MirrorPresent = false;
    int32 _MirrorCandidateIndex = INDEX_NONE;
    FVector _MirrorCandidateVelocity = FVector::ZeroVector;
    FVector _MirrorScoredVelocity = FVector::ZeroVector;
    float _MirrorDesiredVelocityPenalty = 0.0f;
    float _MirrorCurrentVelocityPenalty = 0.0f;
    float _MirrorTimeToImpactPenalty = 0.0f;
    float _MirrorSidePenalty = 0.0f;
    float _MirrorTotalPenalty = 0.0f;
    float _MirrorMinusSelectedPenalty = 0.0f;
    float _TieEpsilon = 0.0f;
    bool _TieWithinEpsilon = false;
};

struct FCk_CrowdDiag_AvoidanceSampleTrace final
{
    int32 _SampledFrame = INDEX_NONE;
    FVector _DesiredVelocity = FVector::ZeroVector;
    FVector _CurrentVelocity = FVector::ZeroVector;
    int32 _NeighborCount = 0;
    float _AgentRadius = 0.0f;
    float _MaxSpeed = 0.0f;
    float _Horizon = 0.0f;
    float _WeightDesiredVelocity = 0.0f;
    float _WeightCurrentVelocity = 0.0f;
    float _WeightTimeToImpact = 0.0f;
    float _WeightSide = 0.0f;
    uint8 _SidePreference = 0;
    FCk_CrowdDiag_TimeToImpactContributor _WinnerTimeToImpactContributor;
    FCk_CrowdDiag_PriorWinnerRescore _PriorWinnerRescore;
    TArray<FCk_CrowdDiag_AvoidanceDepthResult> _Depths;
};

// --------------------------------------------------------------------------------------------------------------------

// Diagnostic recorder data — per-agent breadcrumb trail plus an opt-in, stage-ordered velocity
// pipeline tap. No crowd behavior processor writes this data.
//
// Populated by FProcessor_CrowdAgent_DiagRecorder at the rate set by ck.Crowd.SampleHz
// (default 20Hz). Consumed by:
//   - The diagnostic gym's EmitDigest at end of each cycle → grep-friendly log lines.
//   - The debugger's world-draw overlay → in-viewport breadcrumb polyline so devs can
//     see what an agent actually did on the path it tried to walk.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdDiag_PipelineFrame
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdDiag_PipelineFrame);

public:
    int32 _Frame = 0;

    int32 _RetainedNeighborCount = 0;
    bool _HasNearestNeighbor = false;
    uint32 _NearestHandleHash = 0;
    float _NearestCenterDistance = 0.0f;
    FVector _NearestRelativeOffset = FVector::ZeroVector;
    FVector _NearestRelativeVelocity = FVector::ZeroVector;

    FVector _SeparationForce = FVector::ZeroVector;

    bool _HasWaypointIntent = false;
    int32 _WaypointIndex = INDEX_NONE;
    FVector _WaypointTarget = FVector::ZeroVector;
    FVector _ActiveGoal = FVector::ZeroVector;
    float _ActiveArrivalRadius = 0.0f;
    FVector _SteeringVelocity = FVector::ZeroVector;

    bool _PostAvoidanceOutputObserved = false;
    bool _PostAvoidanceOutputChangedSteering = false;
    FVector _PostAvoidanceOutputVelocity = FVector::ZeroVector;
    bool _AvoidanceScoreObserved = false;
    int32 _AvoidanceSelectedCandidateIndex = INDEX_NONE;
    FVector _AvoidanceSelectedCandidateVelocity = FVector::ZeroVector;
    float _AvoidanceSelectedDesiredVelocityPenalty = 0.0f;
    float _AvoidanceSelectedCurrentVelocityPenalty = 0.0f;
    float _AvoidanceSelectedTimeToImpactPenalty = 0.0f;
    float _AvoidanceSelectedSidePenalty = 0.0f;
    float _AvoidanceSelectedTotalPenalty = 0.0f;
    bool _AvoidanceMirrorPresent = false;
    int32 _AvoidanceMirrorCandidateIndex = INDEX_NONE;
    FVector _AvoidanceMirrorCandidateVelocity = FVector::ZeroVector;
    float _AvoidanceMirrorDesiredVelocityPenalty = 0.0f;
    float _AvoidanceMirrorCurrentVelocityPenalty = 0.0f;
    float _AvoidanceMirrorTimeToImpactPenalty = 0.0f;
    float _AvoidanceMirrorSidePenalty = 0.0f;
    float _AvoidanceMirrorTotalPenalty = 0.0f;
    float _AvoidanceMirrorMinusSelectedPenalty = 0.0f;
    float _AvoidanceTieEpsilon = 0.0f;
    bool _AvoidanceTieWithinEpsilon = false;

    // The clamp's exact pre-clamp desired vector is observed by the preceding avoidance tap. Its
    // previous-frame _LastVelocity is private to AccelClamp and has already been replaced when this
    // tap runs; _AccelLastVelocityAfter is therefore explicitly the post-clamp value.
    FVector _AccelInputVelocity = FVector::ZeroVector;
    FVector _AccelLastVelocityAfter = FVector::ZeroVector;
    FVector _AccelOutputVelocity = FVector::ZeroVector;

    FVector _BridgedVelocity = FVector::ZeroVector;
    FVector _FinalCurrentVelocity = FVector::ZeroVector;
    FVector _PostApplyOffsetDisplacement = FVector::ZeroVector;
    FVector _PushApartContribution = FVector::ZeroVector;
    FVector _PendingDisplacementBeforeConstrain = FVector::ZeroVector;

    // ConstrainToNavmesh queues a transform request rather than writing Transform directly. This
    // is the post-Constrain observed Transform; _AppliedDisplacementSincePriorObservation is the
    // exact physical delta between consecutive recorder observations, not a same-frame claim.
    FVector _PostConstrainObservedPosition = FVector::ZeroVector;
    FVector _AppliedDisplacementSincePriorObservation = FVector::ZeroVector;
    bool _ConstrainConsumedPendingDisplacement = false;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdDiag_PathSample
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdDiag_PathSample);

public:
    // Seconds since tracking started on this agent.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _T = 0.0f;

    // World-space position at sample time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    FVector _Pos = FVector::ZeroVector;

    // Linear speed in cm/s (magnitude of bridged/current velocity).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _Speed = 0.0f;

    // Heading in radians, atan2(VelY, VelX). Meaningless unless _HasDir -- a purely
    // vertical velocity (a spawn-frame fall) has no heading, and atan2(0, 0) returns 0,
    // which reads as "facing +X" rather than "no heading".
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _DirRad = 0.0f;

    // True when the HORIZONTAL velocity was large enough for _DirRad to mean anything.
    // Gated on the 2D magnitude, not _Speed: _Speed is 3D, so a falling agent clears any
    // speed test while contributing no heading at all.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    bool _HasDir = false;

    // Stage outputs captured on the same physics frame as this post-Constrain observation.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    FCk_CrowdDiag_PipelineFrame _Pipeline;

    int32 _AvoidanceSampleTraceIndex = INDEX_NONE;
    int32 _AvoidanceSampleTraceAgeFrames = INDEX_NONE;
    bool _SpatialLoopEligible = false;
    int32 _SpatialLoopWindowId = INDEX_NONE;
    float _TrackGoalDistance = 0.0f;
    float _BestTrackGoalDistance = 0.0f;

public:
    CK_PROPERTY_GET(_T);
    CK_PROPERTY_GET(_Pos);
    CK_PROPERTY_GET(_Speed);
    CK_PROPERTY_GET(_DirRad);
    CK_PROPERTY_GET(_HasDir);
    CK_PROPERTY_GET(_Pipeline);
    CK_PROPERTY_GET(_AvoidanceSampleTraceIndex);
    CK_PROPERTY_GET(_AvoidanceSampleTraceAgeFrames);
    CK_PROPERTY_GET(_SpatialLoopEligible);
    CK_PROPERTY_GET(_SpatialLoopWindowId);
    CK_PROPERTY_GET(_TrackGoalDistance);
    CK_PROPERTY_GET(_BestTrackGoalDistance);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-agent recorder data, added alongside FTag_CrowdDiag_Tracked by Track() at spawn time.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_DiagRecorderData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_DiagRecorderData);

    friend class ck::FProcessor_CrowdAgent_DiagRecorder;
    friend class ck::FProcessor_CrowdAgent_DiagDrawBreadcrumb;
    friend class ck::FProcessor_CrowdAgent_DiagSensorTap;
    friend class ck::FProcessor_CrowdAgent_DiagSeparationTap;
    friend class ck::FProcessor_CrowdAgent_DiagSteeringTap;
    friend class ck::FProcessor_CrowdAgent_DiagAvoidanceTap;
    friend class ck::FProcessor_CrowdAgent_DiagAvoidanceScoreTap;
    friend class ck::FProcessor_CrowdAgent_DiagAccelTap;
    friend class ck::FProcessor_CrowdAgent_DiagVelocityBridgeTap;
    friend class ck::FProcessor_CrowdAgent_DiagApplyOffsetTap;
    friend class ck::FProcessor_CrowdAgent_DiagPushApartTap;
    friend class ::UCk_Utils_CrowdAgent_Diag_UE;

private:
    // Chronological recent-sample buffer. ck.Crowd.SampleHz controls cadence; the oldest quarter
    // is trimmed in one batch when ck.Crowd.MaxRecordedSamples is exceeded.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_CrowdDiag_PathSample> _Samples;

    // Incremented whenever Track() starts a new diagnostic episode. Retained visualization uses
    // this explicit generation to discard the prior episode even when both sample counts are zero.
    int32 _TrackGeneration = 0;

    // Seconds since tracking started; a sample's _T is this value at sample time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _ElapsedSec = 0.0f;

    // Accumulator for the next-sample timer; ticks against ck.Crowd.SampleHz interval.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _SecsSinceLastSample = 0.0f;

    // Spawn position, captured at Track() time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    // Start of the bounded retained sample window. Unlike _StartPos, this advances when old raw
    // diagnostic samples are trimmed and is used only to reconnect retained breadcrumb geometry.
    FVector _RetainedHistoryStartPos = FVector::ZeroVector;

    // Goal location at Track() time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _GoalPos = FVector::ZeroVector;

    // Pivot for angular spatial-loop evidence. This may differ from _GoalPos when a fixture
    // measures circulation around an intersection while progress targets an exit waypoint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _SpatialCenterPos = FVector::ZeroVector;

    float _InitialTrackGoalDistance = 0.0f;
    float _CurrentTrackGoalDistance = 0.0f;
    float _BestTrackGoalDistance = TNumericLimits<float>::Max();

    // Per-cycle metrics — accumulated by the recorder as it samples.

    // Min distance to nearest neighbor across the whole tracked window (across the cluster's
    // 600cm radius the head-on hot moment is what matters; this catches clipping events).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepAcrossCycle = TNumericLimits<float>::Max();

    // Sample t at which _MinSepAcrossCycle was observed.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepTime = 0.0f;

    // Heading flips > 90° between consecutive samples — proxy for the "vibrating" failure mode.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    int32 _DirReversalCount = 0;

    // Max angular delta (degrees) between consecutive samples — catches jitter short of a reversal.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MaxAngularDeltaDeg = 0.0f;

    // True once the agent crossed within an arrival window of _GoalPos. Sticky.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    bool _Reached = false;

    // Sample t at which _Reached first became true.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _TimeToGoal = 0.0f;

    // Live stage slots are reset by the sensor tap once per physics frame and copied into each
    // sampled PathSample by the post-Constrain recorder.
    FCk_CrowdDiag_PipelineFrame _PipelineFrame;
    FCk_CrowdDiag_AvoidanceSampleTrace _LatestAvoidanceSampleTrace;
    TArray<FCk_CrowdDiag_AvoidanceSampleTrace> _AvoidanceSampleTraces;
    FVector _LastObservedPostConstrainPosition = FVector::ZeroVector;
    bool _HasLastObservedPostConstrainPosition = false;

    // Current uninterrupted spatial-loop window around the fixed Track() goal. Velocity heading
    // remains diagnostic only; qualification uses actual post-Constrain positions.
    int32 _CurrentSpatialLoopWindowId = 0;
    bool _SpatialLoopWindowActive = false;
    float _CumulativeSignedSpatialTurnDeg = 0.0f;
    float _CumulativeAbsoluteSpatialTurnDeg = 0.0f;
    float _DominantSpatialTurnDeg = 0.0f;
    float _ReverseSpatialTurnDeg = 0.0f;
    int32 _EligibleSpatialSamples = 0;
    float _FirstEligibleSpatialTime = 0.0f;
    float _LastEligibleSpatialTime = 0.0f;
    float _LastEligibleSpatialAngleRad = 0.0f;
    bool _HasLastEligibleSpatialAngle = false;
    TArray<float> _EligibleSpatialRadii;
    float _EligibleSpatialPathLength = 0.0f;
    FVector _LastEligibleSpatialPosition = FVector::ZeroVector;
    bool _HasLastEligibleSpatialPosition = false;
    FVector _FirstEligibleSpatialPosition = FVector::ZeroVector;
    bool _HasFirstEligibleSpatialPosition = false;
    float _SpatialLoopClosureDistance = 0.0f;
    float _SpatialLoopRepresentativeRadius = 0.0f;
    float _SpatialLoopRequiredPathLength = 0.0f;
    float _SpatialLoopMinRadius = TNumericLimits<float>::Max();
    float _SpatialLoopMaxRadius = 0.0f;
    bool _CurrentSpatialWindowQualified = false;
    bool _SpatialLoopQualified = false;

    // First qualified window is latched so a later arrival or idle gap cannot erase the evidence.
    int32 _QualifiedSpatialLoopWindowId = INDEX_NONE;
    int32 _QualifiedSpatialLoopLastSampleIndex = INDEX_NONE;
    float _QualifiedSpatialTurnSignedDeg = 0.0f;
    float _QualifiedSpatialTurnAbsoluteDeg = 0.0f;
    float _QualifiedSpatialDominantTurnDeg = 0.0f;
    float _QualifiedSpatialReverseTurnDeg = 0.0f;
    int32 _QualifiedSpatialEligibleSamples = 0;
    float _QualifiedSpatialPathLength = 0.0f;
    float _QualifiedSpatialRepresentativeRadius = 0.0f;
    float _QualifiedSpatialRequiredPathLength = 0.0f;
    float _QualifiedSpatialClosureDistance = 0.0f;
    float _QualifiedSpatialWindowNetGoalProgress = 0.0f;
    float _CurrentSpatialWindowStartGoalDistance = 0.0f;

    // Strongest completed *unqualified* window, frozen before the live accumulators clear. This
    // explains near-misses without changing qualification or qualified-window pipeline emission.
    bool _HasBestCompletedSpatialWindow = false;
    int32 _BestCompletedSpatialWindowId = INDEX_NONE;
    float _BestCompletedSpatialTurnSignedDeg = 0.0f;
    float _BestCompletedSpatialTurnAbsoluteDeg = 0.0f;
    float _BestCompletedSpatialDominantTurnDeg = 0.0f;
    float _BestCompletedSpatialReverseTurnDeg = 0.0f;
    int32 _BestCompletedSpatialEligibleSamples = 0;
    float _BestCompletedSpatialElapsedSeconds = 0.0f;
    float _BestCompletedSpatialPathLength = 0.0f;
    float _BestCompletedSpatialRepresentativeRadius = 0.0f;
    float _BestCompletedSpatialMinRadius = TNumericLimits<float>::Max();
    float _BestCompletedSpatialMaxRadius = 0.0f;
    float _BestCompletedSpatialRequiredPathLength = 0.0f;
    float _BestCompletedSpatialClosureDistance = 0.0f;
    float _BestCompletedSpatialWindowNetGoalProgress = 0.0f;
    ECk_CrowdDiag_SpatialWindowResetReason _BestCompletedSpatialResetReason = ECk_CrowdDiag_SpatialWindowResetReason::None;
    float _BestCompletedSpatialResetSpeed = 0.0f;
    float _BestCompletedSpatialResetMinimumSpeed = 0.0f;
    float _BestCompletedSpatialResetRadius = 0.0f;
    float _BestCompletedSpatialResetMinimumRadius = 0.0f;

public:
    CK_PROPERTY_GET(_Samples);
    CK_PROPERTY_GET(_ElapsedSec);
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_GoalPos);
    CK_PROPERTY_GET(_SpatialCenterPos);
    CK_PROPERTY_GET(_InitialTrackGoalDistance);
    CK_PROPERTY_GET(_CurrentTrackGoalDistance);
    CK_PROPERTY_GET(_BestTrackGoalDistance);
    CK_PROPERTY_GET(_MinSepAcrossCycle);
    CK_PROPERTY_GET(_MinSepTime);
    CK_PROPERTY_GET(_DirReversalCount);
    CK_PROPERTY_GET(_MaxAngularDeltaDeg);
    CK_PROPERTY_GET(_Reached);
    CK_PROPERTY_GET(_TimeToGoal);
    CK_PROPERTY_GET(_CurrentSpatialLoopWindowId);
    CK_PROPERTY_GET(_SpatialLoopWindowActive);
    CK_PROPERTY_GET(_CurrentSpatialWindowStartGoalDistance);
    CK_PROPERTY_GET(_CumulativeSignedSpatialTurnDeg);
    CK_PROPERTY_GET(_CumulativeAbsoluteSpatialTurnDeg);
    CK_PROPERTY_GET(_DominantSpatialTurnDeg);
    CK_PROPERTY_GET(_ReverseSpatialTurnDeg);
    CK_PROPERTY_GET(_EligibleSpatialSamples);
    CK_PROPERTY_GET(_EligibleSpatialPathLength);
    CK_PROPERTY_GET(_SpatialLoopClosureDistance);
    CK_PROPERTY_GET(_SpatialLoopRepresentativeRadius);
    CK_PROPERTY_GET(_SpatialLoopRequiredPathLength);
    CK_PROPERTY_GET(_SpatialLoopMinRadius);
    CK_PROPERTY_GET(_SpatialLoopMaxRadius);
    CK_PROPERTY_GET(_SpatialLoopQualified);
    CK_PROPERTY_GET(_HasBestCompletedSpatialWindow);
    CK_PROPERTY_GET(_BestCompletedSpatialDominantTurnDeg);
    CK_PROPERTY_GET(_QualifiedSpatialLoopWindowId);
    CK_PROPERTY_GET(_QualifiedSpatialLoopLastSampleIndex);
    CK_PROPERTY_GET(_QualifiedSpatialTurnSignedDeg);
    CK_PROPERTY_GET(_QualifiedSpatialTurnAbsoluteDeg);
    CK_PROPERTY_GET(_QualifiedSpatialDominantTurnDeg);
    CK_PROPERTY_GET(_QualifiedSpatialReverseTurnDeg);
    CK_PROPERTY_GET(_QualifiedSpatialEligibleSamples);
    CK_PROPERTY_GET(_QualifiedSpatialPathLength);
    CK_PROPERTY_GET(_QualifiedSpatialRepresentativeRadius);
    CK_PROPERTY_GET(_QualifiedSpatialRequiredPathLength);
    CK_PROPERTY_GET(_QualifiedSpatialClosureDistance);
    CK_PROPERTY_GET(_QualifiedSpatialWindowNetGoalProgress);
};

// --------------------------------------------------------------------------------------------------------------------
