#include "CkCrowdAgent_Diag_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Track(
        FCk_Handle_CrowdAgent& InAgent,
        FVector InStartPos,
        FVector InGoalPos)
    -> FCk_Handle_CrowdAgent
{
    return TrackWithSpatialCenter(InAgent, InStartPos, InGoalPos, InGoalPos);
}

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    TrackWithSpatialCenter(
        FCk_Handle_CrowdAgent& InAgent,
        FVector InStartPos,
        FVector InProgressGoalPos,
        FVector InSpatialCenterPos)
    -> FCk_Handle_CrowdAgent
{
    const auto IsValidAgent = ck::IsValid(InAgent);
    CK_ENSURE_IF_NOT(IsValidAgent,
        TEXT("Invalid CrowdAgent handle [{}] passed to TrackWithSpatialCenter"), InAgent)
    {
        return InAgent;
    }

    InAgent.AddOrGet<ck::FTag_CrowdDiag_Tracked>();

    auto& Recorder = InAgent.AddOrGet<ck::FFragment_CrowdAgent_DiagRecorder>();
    ++Recorder._TrackGeneration;
    Recorder._Samples.Reset();
    Recorder._ElapsedSec = 0.0f;
    Recorder._SecsSinceLastSample = 0.0f;
    Recorder._StartPos = InStartPos;
    Recorder._RetainedHistoryStartPos = InStartPos;
    Recorder._GoalPos = InProgressGoalPos;
    Recorder._SpatialCenterPos = InSpatialCenterPos;
    Recorder._InitialTrackGoalDistance = FVector::Dist2D(InStartPos, InProgressGoalPos);
    Recorder._CurrentTrackGoalDistance = Recorder._InitialTrackGoalDistance;
    Recorder._BestTrackGoalDistance = Recorder._InitialTrackGoalDistance;
    Recorder._MinSepAcrossCycle = TNumericLimits<float>::Max();
    Recorder._MinSepTime = 0.0f;
    Recorder._DirReversalCount = 0;
    Recorder._MaxAngularDeltaDeg = 0.0f;
    Recorder._Reached = false;
    Recorder._TimeToGoal = 0.0f;
    Recorder._PipelineFrame = {};
    Recorder._LatestAvoidanceSampleTrace = {};
    Recorder._AvoidanceSampleTraces.Reset();
    Recorder._LastObservedPostConstrainPosition = FVector::ZeroVector;
    Recorder._HasLastObservedPostConstrainPosition = false;
    Recorder._CurrentSpatialLoopWindowId = 0;
    Recorder._SpatialLoopWindowActive = false;
    Recorder._CumulativeSignedSpatialTurnDeg = 0.0f;
    Recorder._CumulativeAbsoluteSpatialTurnDeg = 0.0f;
    Recorder._DominantSpatialTurnDeg = 0.0f;
    Recorder._ReverseSpatialTurnDeg = 0.0f;
    Recorder._EligibleSpatialSamples = 0;
    Recorder._FirstEligibleSpatialTime = 0.0f;
    Recorder._LastEligibleSpatialTime = 0.0f;
    Recorder._LastEligibleSpatialAngleRad = 0.0f;
    Recorder._HasLastEligibleSpatialAngle = false;
    Recorder._EligibleSpatialRadii.Reset();
    Recorder._EligibleSpatialPathLength = 0.0f;
    Recorder._LastEligibleSpatialPosition = FVector::ZeroVector;
    Recorder._HasLastEligibleSpatialPosition = false;
    Recorder._FirstEligibleSpatialPosition = FVector::ZeroVector;
    Recorder._HasFirstEligibleSpatialPosition = false;
    Recorder._SpatialLoopClosureDistance = 0.0f;
    Recorder._SpatialLoopRepresentativeRadius = 0.0f;
    Recorder._SpatialLoopRequiredPathLength = 0.0f;
    Recorder._SpatialLoopMinRadius = TNumericLimits<float>::Max();
    Recorder._SpatialLoopMaxRadius = 0.0f;
    Recorder._CurrentSpatialWindowQualified = false;

    Recorder._SpatialLoopQualified = false;
    Recorder._QualifiedSpatialLoopWindowId = INDEX_NONE;
    Recorder._QualifiedSpatialLoopLastSampleIndex = INDEX_NONE;
    Recorder._QualifiedSpatialTurnSignedDeg = 0.0f;
    Recorder._QualifiedSpatialTurnAbsoluteDeg = 0.0f;
    Recorder._QualifiedSpatialDominantTurnDeg = 0.0f;
    Recorder._QualifiedSpatialReverseTurnDeg = 0.0f;
    Recorder._QualifiedSpatialEligibleSamples = 0;
    Recorder._QualifiedSpatialPathLength = 0.0f;
    Recorder._QualifiedSpatialRepresentativeRadius = 0.0f;
    Recorder._QualifiedSpatialRequiredPathLength = 0.0f;
    Recorder._QualifiedSpatialClosureDistance = 0.0f;
    Recorder._QualifiedSpatialWindowNetGoalProgress = 0.0f;
    Recorder._CurrentSpatialWindowStartGoalDistance = 0.0f;
    Recorder._HasBestCompletedSpatialWindow = false;
    Recorder._BestCompletedSpatialWindowId = INDEX_NONE;
    Recorder._BestCompletedSpatialTurnSignedDeg = 0.0f;
    Recorder._BestCompletedSpatialTurnAbsoluteDeg = 0.0f;
    Recorder._BestCompletedSpatialDominantTurnDeg = 0.0f;
    Recorder._BestCompletedSpatialReverseTurnDeg = 0.0f;
    Recorder._BestCompletedSpatialEligibleSamples = 0;
    Recorder._BestCompletedSpatialElapsedSeconds = 0.0f;
    Recorder._BestCompletedSpatialPathLength = 0.0f;
    Recorder._BestCompletedSpatialRepresentativeRadius = 0.0f;
    Recorder._BestCompletedSpatialMinRadius = TNumericLimits<float>::Max();
    Recorder._BestCompletedSpatialMaxRadius = 0.0f;
    Recorder._BestCompletedSpatialRequiredPathLength = 0.0f;
    Recorder._BestCompletedSpatialClosureDistance = 0.0f;
    Recorder._BestCompletedSpatialWindowNetGoalProgress = 0.0f;
    Recorder._BestCompletedSpatialResetReason = ECk_CrowdDiag_SpatialWindowResetReason::None;
    Recorder._BestCompletedSpatialResetSpeed = 0.0f;
    Recorder._BestCompletedSpatialResetMinimumSpeed = 0.0f;
    Recorder._BestCompletedSpatialResetRadius = 0.0f;
    Recorder._BestCompletedSpatialResetMinimumRadius = 0.0f;

    InAgent.AddOrGet<ck::FFragment_CrowdAgent_DiagBreadcrumb>();

    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Is_Tracked(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    if (ck::Is_NOT_Valid(InAgent))
    { return false; }
    return InAgent.Has<ck::FTag_CrowdDiag_Tracked>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Get_RecorderData(
        const FCk_Handle_CrowdAgent& InAgent)
    -> FCk_Fragment_CrowdAgent_DiagRecorderData
{
    if (ck::Is_NOT_Valid(InAgent))
    { return {}; }
    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_DiagRecorder>())
    { return {}; }
    return InAgent.Get<ck::FFragment_CrowdAgent_DiagRecorder>();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_utils
{
    constexpr auto RecentPipelineLookbackSeconds = 5.0f;

    auto ToResetReasonString(ECk_CrowdDiag_SpatialWindowResetReason InReason) -> const TCHAR*
    {
        switch (InReason)
        {
        case ECk_CrowdDiag_SpatialWindowResetReason::Reached: return TEXT("reached");
        case ECk_CrowdDiag_SpatialWindowResetReason::SpeedBelowMinimum: return TEXT("speed_below_20");
        case ECk_CrowdDiag_SpatialWindowResetReason::RadiusBelowMinimum: return TEXT("radius_below_active_arrival_plus_10");
        default: return TEXT("none");
        }
    }

    // RDP epsilon — perpendicular-distance tolerance in cm for path simplification. Lower =
    // more keypoints retained (more detail in the digest); higher = more aggressive collapse.
    // 8cm chosen so a straight head-on test produces ~2-3 keypoints and a curving cluster path
    // produces ~10-20 — enough to read the shape, light enough to grep without paging.
    static TAutoConsoleVariable<float> CVarRDPEpsilon(
        TEXT("ck.Crowd.RDPEpsilon"),
        8.0f,
        TEXT("Ramer-Douglas-Peucker epsilon (cm) for path-simplification in the cycle digest.\n")
        TEXT("Lower = more keypoints kept; higher = more aggressive collapse. Default 8cm."),
        ECVF_Default);

    auto PerpendicularDistanceXY(const FVector& Point, const FVector& LineStart, const FVector& LineEnd) -> float
    {
        const auto Line = FVector(LineEnd.X - LineStart.X, LineEnd.Y - LineStart.Y, 0.0);
        const auto LineSqLen = Line.SizeSquared();
        if (LineSqLen < KINDA_SMALL_NUMBER)
        {
            const auto Dx = Point.X - LineStart.X;
            const auto Dy = Point.Y - LineStart.Y;
            return static_cast<float>(FMath::Sqrt(Dx * Dx + Dy * Dy));
        }
        const auto ToPt = FVector(Point.X - LineStart.X, Point.Y - LineStart.Y, 0.0);
        const auto T = FMath::Clamp(static_cast<float>(FVector::DotProduct(ToPt, Line) / LineSqLen), 0.0f, 1.0f);
        const auto Closest = FVector(LineStart.X + Line.X * T, LineStart.Y + Line.Y * T, 0.0);
        const auto Dx = Point.X - Closest.X;
        const auto Dy = Point.Y - Closest.Y;
        return static_cast<float>(FMath::Sqrt(Dx * Dx + Dy * Dy));
    }

    // Recursive RDP. Marks indices that should be kept by setting OutKeep[i] = true. Caller
    // pre-marks endpoints; this only fills the interior. Iterative form would be marginally
    // faster but at our sample counts (≤180 per agent per cycle) the recursion is irrelevant.
    auto RDP_Recursive(
        const TArray<FCk_CrowdDiag_PathSample>& InSamples,
        int32 InStartIdx,
        int32 InEndIdx,
        float InEpsilon,
        TArray<bool>& OutKeep) -> void
    {
        if (InEndIdx - InStartIdx < 2)
        { return; }

        auto MaxDist = 0.0f;
        auto MaxIdx = InStartIdx;
        for (auto i = InStartIdx + 1; i < InEndIdx; ++i)
        {
            const auto D = PerpendicularDistanceXY(
                InSamples[i].Get_Pos(),
                InSamples[InStartIdx].Get_Pos(),
                InSamples[InEndIdx].Get_Pos());
            if (D > MaxDist)
            {
                MaxDist = D;
                MaxIdx = i;
            }
        }

        if (MaxDist > InEpsilon)
        {
            OutKeep[MaxIdx] = true;
            RDP_Recursive(InSamples, InStartIdx, MaxIdx, InEpsilon, OutKeep);
            RDP_Recursive(InSamples, MaxIdx, InEndIdx, InEpsilon, OutKeep);
        }
    }
}

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    EmitDigest_ForAgent(
        const FCk_Handle_CrowdAgent& InAgent,
        int32 InCycleNumber,
        const FString& InStationName,
        int32 InAgentIndex,
        bool InEmitRecentPipeline)
    -> void
{
    if (ck::Is_NOT_Valid(InAgent))
    { return; }
    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_DiagRecorder>())
    { return; }

    const auto& Recorder = InAgent.Get<ck::FFragment_CrowdAgent_DiagRecorder>();
    const auto& Samples = Recorder.Get_Samples();

    const auto Prefix = FString::Printf(TEXT("[CrowdDiag][C%d][%s][A%d]"),
        InCycleNumber, *InStationName, InAgentIndex);

    auto PathLen = 0.0;
    for (auto i = 1; i < Samples.Num(); ++i)
    {
        const auto& A = Samples[i - 1].Get_Pos();
        const auto& B = Samples[i].Get_Pos();
        const auto Dx = B.X - A.X;
        const auto Dy = B.Y - A.Y;
        PathLen += FMath::Sqrt(Dx * Dx + Dy * Dy);
    }
    const auto Straight = FVector::Dist(Recorder.Get_StartPos(), Recorder.Get_GoalPos());
    const auto Efficiency = (PathLen > KINDA_SMALL_NUMBER) ? (Straight / PathLen) : 0.0;

    // Emit the header lines. ck::crowd::Display routes through LogCk_Crowd at Display verbosity
    // (visible by default in Saved/Logs/CkTests.log without bumping LogCk_Crowd to Verbose).
    ck::crowd::Display(TEXT("{} start=({:.1f}, {:.1f}, {:.1f}) goal=({:.1f}, {:.1f}, {:.1f}) spatial_center=({:.1f}, {:.1f}, {:.1f})"),
        Prefix,
        Recorder.Get_StartPos().X, Recorder.Get_StartPos().Y, Recorder.Get_StartPos().Z,
        Recorder.Get_GoalPos().X, Recorder.Get_GoalPos().Y, Recorder.Get_GoalPos().Z,
        Recorder.Get_SpatialCenterPos().X, Recorder.Get_SpatialCenterPos().Y, Recorder.Get_SpatialCenterPos().Z);

    ck::crowd::Display(TEXT("{} reached={} t_to_goal={:.2f}"),
        Prefix,
        Recorder.Get_Reached() ? TEXT("true") : TEXT("false"),
        Recorder.Get_TimeToGoal());

    ck::crowd::Display(TEXT("{} path_len={:.1f} straight={:.1f} efficiency={:.3f}"),
        Prefix, PathLen, Straight, Efficiency);

    constexpr auto NoNeighboursObservedSentinel = -1.0f;
    const auto MinSep = (Recorder.Get_MinSepAcrossCycle() == TNumericLimits<float>::Max())
        ? NoNeighboursObservedSentinel
        : Recorder.Get_MinSepAcrossCycle();
    ck::crowd::Display(TEXT("{} nearest_center_distance={:.1f} at t={:.2f}"),
        Prefix, MinSep, Recorder.Get_MinSepTime());

    ck::crowd::Display(TEXT("{} dir_reversals={} max_angular_delta={:.1f}"),
        Prefix, Recorder.Get_DirReversalCount(), Recorder.Get_MaxAngularDeltaDeg());

    ck::crowd::Display(TEXT("{} track_goal_distance initial={:.1f} current={:.1f} best={:.1f}"),
        Prefix,
        Recorder.Get_InitialTrackGoalDistance(),
        Recorder.Get_CurrentTrackGoalDistance(),
        Recorder.Get_BestTrackGoalDistance());

    const auto CurrentWindowNetGoalProgress = Recorder.Get_SpatialLoopWindowActive()
        ? Recorder.Get_CurrentSpatialWindowStartGoalDistance() - Recorder.Get_CurrentTrackGoalDistance()
        : 0.0f;
    ck::crowd::Display(TEXT("{} spatial_window active={} id={} signed={:.1f} abs={:.1f} dominant={:.1f} reverse={:.1f} samples={} path_len={:.1f} radius={:.1f} radius_band=[{:.1f},{:.1f}] required_path_len={:.1f} closure={:.1f} net_goal_progress={:.1f}"),
        Prefix,
        Recorder.Get_SpatialLoopWindowActive() ? TEXT("true") : TEXT("false"),
        Recorder.Get_CurrentSpatialLoopWindowId(),
        Recorder.Get_CumulativeSignedSpatialTurnDeg(),
        Recorder.Get_CumulativeAbsoluteSpatialTurnDeg(),
        Recorder.Get_DominantSpatialTurnDeg(),
        Recorder.Get_ReverseSpatialTurnDeg(),
        Recorder.Get_EligibleSpatialSamples(),
        Recorder.Get_EligibleSpatialPathLength(),
        Recorder.Get_SpatialLoopRepresentativeRadius(),
        Recorder.Get_SpatialLoopMinRadius() == TNumericLimits<float>::Max() ? -1.0f : Recorder.Get_SpatialLoopMinRadius(),
        Recorder.Get_SpatialLoopMaxRadius(),
        Recorder.Get_SpatialLoopRequiredPathLength(),
        Recorder.Get_SpatialLoopClosureDistance(),
        CurrentWindowNetGoalProgress);

    ck::crowd::Display(TEXT("{} spatial_loop qualified={} qualified_window={} signed={:.1f} abs={:.1f} dominant={:.1f} reverse={:.1f} samples={} path_len={:.1f} radius={:.1f} required_path_len={:.1f} closure={:.1f} net_goal_progress={:.1f}"),
        Prefix,
        Recorder.Get_SpatialLoopQualified() ? TEXT("true") : TEXT("false"),
        Recorder.Get_QualifiedSpatialLoopWindowId(),
        Recorder.Get_QualifiedSpatialTurnSignedDeg(),
        Recorder.Get_QualifiedSpatialTurnAbsoluteDeg(),
        Recorder.Get_QualifiedSpatialDominantTurnDeg(),
        Recorder.Get_QualifiedSpatialReverseTurnDeg(),
        Recorder.Get_QualifiedSpatialEligibleSamples(),
        Recorder.Get_QualifiedSpatialPathLength(),
        Recorder.Get_QualifiedSpatialRepresentativeRadius(),
        Recorder.Get_QualifiedSpatialRequiredPathLength(),
        Recorder.Get_QualifiedSpatialClosureDistance(),
        Recorder.Get_QualifiedSpatialWindowNetGoalProgress());

    if (Recorder._HasBestCompletedSpatialWindow)
    {
        ck::crowd::Display(TEXT("{} spatial_best_completed window={} reset_reason={} reset_speed={:.1f} min_speed={:.1f} reset_radius={:.1f} min_radius={:.1f} signed={:.1f} abs={:.1f} dominant={:.1f} reverse={:.1f} samples={} elapsed={:.2f} path_len={:.1f} radius={:.1f} radius_band=[{:.1f},{:.1f}] required_path_len={:.1f} closure={:.1f} net_goal_progress={:.1f}"),
            Prefix,
            Recorder._BestCompletedSpatialWindowId,
            ck_crowd_agent_diag_utils::ToResetReasonString(Recorder._BestCompletedSpatialResetReason),
            Recorder._BestCompletedSpatialResetSpeed,
            Recorder._BestCompletedSpatialResetMinimumSpeed,
            Recorder._BestCompletedSpatialResetRadius,
            Recorder._BestCompletedSpatialResetMinimumRadius,
            Recorder._BestCompletedSpatialTurnSignedDeg,
            Recorder._BestCompletedSpatialTurnAbsoluteDeg,
            Recorder._BestCompletedSpatialDominantTurnDeg,
            Recorder._BestCompletedSpatialReverseTurnDeg,
            Recorder._BestCompletedSpatialEligibleSamples,
            Recorder._BestCompletedSpatialElapsedSeconds,
            Recorder._BestCompletedSpatialPathLength,
            Recorder._BestCompletedSpatialRepresentativeRadius,
            Recorder._BestCompletedSpatialMinRadius == TNumericLimits<float>::Max() ? -1.0f : Recorder._BestCompletedSpatialMinRadius,
            Recorder._BestCompletedSpatialMaxRadius,
            Recorder._BestCompletedSpatialRequiredPathLength,
            Recorder._BestCompletedSpatialClosureDistance,
            Recorder._BestCompletedSpatialWindowNetGoalProgress);
    }

    const auto EmitRecentPipeline = InEmitRecentPipeline;
    const auto EmitQualifiedPipeline =
        Recorder.Get_SpatialLoopQualified() && NOT EmitRecentPipeline;
    if (EmitQualifiedPipeline || EmitRecentPipeline)
    {
        const auto PipelineEndTime = Samples.IsEmpty() ? 0.0f : Samples.Last().Get_T();
        const auto PipelineStartTime = EmitRecentPipeline
            ? FMath::Max(
                0.0f,
                PipelineEndTime - ck_crowd_agent_diag_utils::RecentPipelineLookbackSeconds)
            : 0.0f;
        ck::crowd::Display(TEXT("{} pipeline_window mode={} start_t={:.2f} end_t={:.2f}"),
            Prefix,
            EmitQualifiedPipeline ? TEXT("qualified") : TEXT("recent"),
            PipelineStartTime,
            PipelineEndTime);

        int32 LastEmittedAvoidanceSampleFrame = INDEX_NONE;
        for (auto SampleIndex = 0; SampleIndex < Samples.Num(); ++SampleIndex)
        {
            const auto& S = Samples[SampleIndex];
            if (EmitQualifiedPipeline)
            {
                if (S.Get_SpatialLoopWindowId() != Recorder.Get_QualifiedSpatialLoopWindowId())
                { continue; }
                if (SampleIndex > Recorder.Get_QualifiedSpatialLoopLastSampleIndex())
                { break; }
            }
            else if (S.Get_T() < PipelineStartTime)
            {
                continue;
            }

            const auto& P = S.Get_Pipeline();
            ck::crowd::Display(TEXT("{} pipeline t={:.2f} frame={} window={} track_goal_distance={:.1f} best_track_goal_distance={:.1f} neighbors={} nearest_handle_hash={} nearest_center_distance={:.1f} rel_offset=({:.1f},{:.1f},{:.1f}) rel_velocity=({:.1f},{:.1f},{:.1f}) sep=({:.1f},{:.1f},{:.1f})"),
                Prefix, S.Get_T(), P._Frame, S.Get_SpatialLoopWindowId(), S.Get_TrackGoalDistance(), S.Get_BestTrackGoalDistance(),
                P._RetainedNeighborCount, P._NearestHandleHash, P._NearestCenterDistance,
                P._NearestRelativeOffset.X, P._NearestRelativeOffset.Y, P._NearestRelativeOffset.Z,
                P._NearestRelativeVelocity.X, P._NearestRelativeVelocity.Y, P._NearestRelativeVelocity.Z,
                P._SeparationForce.X, P._SeparationForce.Y, P._SeparationForce.Z);
            ck::crowd::Display(TEXT("{} pipeline waypoint_index={} waypoint=({:.1f},{:.1f},{:.1f}) goal=({:.1f},{:.1f},{:.1f}) steering=({:.1f},{:.1f},{:.1f}) post_avoidance_output_observed={} post_avoidance_output_changed_steering={} post_avoidance_output=({:.1f},{:.1f},{:.1f})"),
                Prefix,
                P._WaypointIndex, P._WaypointTarget.X, P._WaypointTarget.Y, P._WaypointTarget.Z,
                P._ActiveGoal.X, P._ActiveGoal.Y, P._ActiveGoal.Z,
                P._SteeringVelocity.X, P._SteeringVelocity.Y, P._SteeringVelocity.Z,
                P._PostAvoidanceOutputObserved ? TEXT("true") : TEXT("false"),
                P._PostAvoidanceOutputChangedSteering ? TEXT("true") : TEXT("false"),
                P._PostAvoidanceOutputVelocity.X, P._PostAvoidanceOutputVelocity.Y, P._PostAvoidanceOutputVelocity.Z);
            ck::crowd::Display(TEXT("{} pipeline accel_input=({:.1f},{:.1f},{:.1f}) accel_last_after=({:.1f},{:.1f},{:.1f}) accel_output=({:.1f},{:.1f},{:.1f}) bridged=({:.1f},{:.1f},{:.1f}) final_current=({:.1f},{:.1f},{:.1f})"),
                Prefix,
                P._AccelInputVelocity.X, P._AccelInputVelocity.Y, P._AccelInputVelocity.Z,
                P._AccelLastVelocityAfter.X, P._AccelLastVelocityAfter.Y, P._AccelLastVelocityAfter.Z,
                P._AccelOutputVelocity.X, P._AccelOutputVelocity.Y, P._AccelOutputVelocity.Z,
                P._BridgedVelocity.X, P._BridgedVelocity.Y, P._BridgedVelocity.Z,
                P._FinalCurrentVelocity.X, P._FinalCurrentVelocity.Y, P._FinalCurrentVelocity.Z);
            ck::crowd::Display(
                TEXT("{} pipeline avoidance_score observed={} selected_index={} selected_penalty={:.6f} selected_components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] mirror_present={} mirror_index={} mirror_penalty={:.6f} mirror_components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] mirror_delta={:.6f} tie_epsilon={:.6f} tie={}"),
                Prefix,
                P._AvoidanceScoreObserved ? TEXT("true") : TEXT("false"),
                P._AvoidanceSelectedCandidateIndex,
                P._AvoidanceSelectedTotalPenalty,
                P._AvoidanceSelectedDesiredVelocityPenalty,
                P._AvoidanceSelectedCurrentVelocityPenalty,
                P._AvoidanceSelectedTimeToImpactPenalty,
                P._AvoidanceSelectedSidePenalty,
                P._AvoidanceMirrorPresent ? TEXT("true") : TEXT("false"),
                P._AvoidanceMirrorCandidateIndex,
                P._AvoidanceMirrorTotalPenalty,
                P._AvoidanceMirrorDesiredVelocityPenalty,
                P._AvoidanceMirrorCurrentVelocityPenalty,
                P._AvoidanceMirrorTimeToImpactPenalty,
                P._AvoidanceMirrorSidePenalty,
                P._AvoidanceMirrorMinusSelectedPenalty,
                P._AvoidanceTieEpsilon,
                P._AvoidanceTieWithinEpsilon ? TEXT("true") : TEXT("false"));

            const auto TraceIndex = S.Get_AvoidanceSampleTraceIndex();
            if (Recorder._AvoidanceSampleTraces.IsValidIndex(TraceIndex))
            {
                const auto& Trace = Recorder._AvoidanceSampleTraces[TraceIndex];
                if (Trace._SampledFrame != LastEmittedAvoidanceSampleFrame)
                {
                    ck::crowd::Display(
                        TEXT("{} sampler_trace t={:.2f} recorder_frame={} sampled_frame={} age_frames={} depths={}"),
                        Prefix,
                        S.Get_T(),
                        P._Frame,
                        Trace._SampledFrame,
                        S.Get_AvoidanceSampleTraceAgeFrames(),
                        Trace._Depths.Num());
                    ck::crowd::Display(
                        TEXT("{} sampler_inputs sampled_frame={} desired=({:.3f},{:.3f},{:.3f}) current=({:.3f},{:.3f},{:.3f}) neighbors={} agent_radius={:.3f} max_speed={:.3f} horizon={:.3f} weights=[des={:.3f},cur={:.3f},toi={:.3f},side={:.3f}] side_preference={}"),
                        Prefix,
                        Trace._SampledFrame,
                        Trace._DesiredVelocity.X,
                        Trace._DesiredVelocity.Y,
                        Trace._DesiredVelocity.Z,
                        Trace._CurrentVelocity.X,
                        Trace._CurrentVelocity.Y,
                        Trace._CurrentVelocity.Z,
                        Trace._NeighborCount,
                        Trace._AgentRadius,
                        Trace._MaxSpeed,
                        Trace._Horizon,
                        Trace._WeightDesiredVelocity,
                        Trace._WeightCurrentVelocity,
                        Trace._WeightTimeToImpact,
                        Trace._WeightSide,
                        static_cast<int32>(Trace._SidePreference));
                    const auto& WinnerToi = Trace._WinnerTimeToImpactContributor;
                    ck::crowd::Display(
                        TEXT("{} sampler_winner_toi sampled_frame={} observed={} neighbor_index={} neighbor_hash={} time={:.6f} relative_offset=({:.3f},{:.3f},{:.3f}) relative_velocity=({:.3f},{:.3f},{:.3f})"),
                        Prefix,
                        Trace._SampledFrame,
                        WinnerToi._Observed ? TEXT("true") : TEXT("false"),
                        WinnerToi._NeighborIndex,
                        WinnerToi._NeighborHandleHash,
                        WinnerToi._Time,
                        WinnerToi._RelativeOffset.X,
                        WinnerToi._RelativeOffset.Y,
                        WinnerToi._RelativeOffset.Z,
                        WinnerToi._RelativeVelocity.X,
                        WinnerToi._RelativeVelocity.Y,
                        WinnerToi._RelativeVelocity.Z);
                    const auto& Prior = Trace._PriorWinnerRescore;
                    ck::crowd::Display(
                        TEXT("{} sampler_prior sampled_frame={} observed={} source_frame={} age_frames={} raw_velocity=({:.3f},{:.3f},{:.3f}) scored_velocity=({:.3f},{:.3f},{:.3f}) heading_delta={:.3f} penalty={:.6f} components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] minus_winner={:.6f} toi_observed={} toi_neighbor_index={} toi_neighbor_hash={} toi_time={:.6f} toi_relative_offset=({:.3f},{:.3f},{:.3f}) toi_relative_velocity=({:.3f},{:.3f},{:.3f})"),
                        Prefix,
                        Trace._SampledFrame,
                        Prior._Observed ? TEXT("true") : TEXT("false"),
                        Prior._SourceSampledFrame,
                        Prior._AgeFrames,
                        Prior._RawVelocity.X,
                        Prior._RawVelocity.Y,
                        Prior._RawVelocity.Z,
                        Prior._ScoredVelocity.X,
                        Prior._ScoredVelocity.Y,
                        Prior._ScoredVelocity.Z,
                        Prior._HeadingDeltaToCurrentWinnerDeg,
                        Prior._Score._TotalPenalty,
                        Prior._Score._DesiredVelocityPenalty,
                        Prior._Score._CurrentVelocityPenalty,
                        Prior._Score._TimeToImpactPenalty,
                        Prior._Score._SidePenalty,
                        Prior._MinusCurrentWinnerPenalty,
                        Prior._TimeToImpactContributor._Observed ? TEXT("true") : TEXT("false"),
                        Prior._TimeToImpactContributor._NeighborIndex,
                        Prior._TimeToImpactContributor._NeighborHandleHash,
                        Prior._TimeToImpactContributor._Time,
                        Prior._TimeToImpactContributor._RelativeOffset.X,
                        Prior._TimeToImpactContributor._RelativeOffset.Y,
                        Prior._TimeToImpactContributor._RelativeOffset.Z,
                        Prior._TimeToImpactContributor._RelativeVelocity.X,
                        Prior._TimeToImpactContributor._RelativeVelocity.Y,
                        Prior._TimeToImpactContributor._RelativeVelocity.Z);
                    ck::crowd::Display(
                        TEXT("{} sampler_prior_nearest sampled_frame={} observed={} candidate_index={} raw_candidate_velocity=({:.3f},{:.3f},{:.3f}) scored_candidate_velocity=({:.3f},{:.3f},{:.3f}) distance={:.6f} penalty={:.6f} components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] minus_winner={:.6f} toi_observed={} toi_neighbor_index={} toi_neighbor_hash={} toi_time={:.6f} toi_relative_offset=({:.3f},{:.3f},{:.3f}) toi_relative_velocity=({:.3f},{:.3f},{:.3f})"),
                        Prefix,
                        Trace._SampledFrame,
                        Prior._Observed ? TEXT("true") : TEXT("false"),
                        Prior._NearestFinalCandidateIndex,
                        Prior._NearestFinalCandidateRawVelocity.X,
                        Prior._NearestFinalCandidateRawVelocity.Y,
                        Prior._NearestFinalCandidateRawVelocity.Z,
                        Prior._NearestFinalCandidateScoredVelocity.X,
                        Prior._NearestFinalCandidateScoredVelocity.Y,
                        Prior._NearestFinalCandidateScoredVelocity.Z,
                        Prior._NearestFinalCandidateDistance,
                        Prior._NearestFinalCandidateScore._TotalPenalty,
                        Prior._NearestFinalCandidateScore._DesiredVelocityPenalty,
                        Prior._NearestFinalCandidateScore._CurrentVelocityPenalty,
                        Prior._NearestFinalCandidateScore._TimeToImpactPenalty,
                        Prior._NearestFinalCandidateScore._SidePenalty,
                        Prior._NearestFinalCandidateMinusWinnerPenalty,
                        Prior._NearestFinalCandidateTimeToImpactContributor._Observed ? TEXT("true") : TEXT("false"),
                        Prior._NearestFinalCandidateTimeToImpactContributor._NeighborIndex,
                        Prior._NearestFinalCandidateTimeToImpactContributor._NeighborHandleHash,
                        Prior._NearestFinalCandidateTimeToImpactContributor._Time,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeOffset.X,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeOffset.Y,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeOffset.Z,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeVelocity.X,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeVelocity.Y,
                        Prior._NearestFinalCandidateTimeToImpactContributor._RelativeVelocity.Z);
                    for (const auto& Depth : Trace._Depths)
                    {
                        ck::crowd::Display(
                            TEXT("{} sampler_depth sampled_frame={} depth={} center=({:.3f},{:.3f},{:.3f}) radius={:.3f} selected_index={} selected_velocity=({:.3f},{:.3f},{:.3f}) selected_scored_velocity=({:.3f},{:.3f},{:.3f}) selected_penalty={:.6f} selected_components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] mirror_present={} mirror_index={} mirror_velocity=({:.3f},{:.3f},{:.3f}) mirror_scored_velocity=({:.3f},{:.3f},{:.3f}) mirror_penalty={:.6f} mirror_components=[des={:.6f},cur={:.6f},toi={:.6f},side={:.6f}] mirror_delta={:.6f} tie_epsilon={:.6f} tie={}"),
                            Prefix,
                            Trace._SampledFrame,
                            Depth._Depth,
                            Depth._Centre.X, Depth._Centre.Y, Depth._Centre.Z,
                            Depth._Radius,
                            Depth._SelectedCandidateIndex,
                            Depth._SelectedCandidateVelocity.X,
                            Depth._SelectedCandidateVelocity.Y,
                            Depth._SelectedCandidateVelocity.Z,
                            Depth._SelectedScoredVelocity.X,
                            Depth._SelectedScoredVelocity.Y,
                            Depth._SelectedScoredVelocity.Z,
                            Depth._SelectedTotalPenalty,
                            Depth._SelectedDesiredVelocityPenalty,
                            Depth._SelectedCurrentVelocityPenalty,
                            Depth._SelectedTimeToImpactPenalty,
                            Depth._SelectedSidePenalty,
                            Depth._MirrorPresent ? TEXT("true") : TEXT("false"),
                            Depth._MirrorCandidateIndex,
                            Depth._MirrorCandidateVelocity.X,
                            Depth._MirrorCandidateVelocity.Y,
                            Depth._MirrorCandidateVelocity.Z,
                            Depth._MirrorScoredVelocity.X,
                            Depth._MirrorScoredVelocity.Y,
                            Depth._MirrorScoredVelocity.Z,
                            Depth._MirrorTotalPenalty,
                            Depth._MirrorDesiredVelocityPenalty,
                            Depth._MirrorCurrentVelocityPenalty,
                            Depth._MirrorTimeToImpactPenalty,
                            Depth._MirrorSidePenalty,
                            Depth._MirrorMinusSelectedPenalty,
                            Depth._TieEpsilon,
                            Depth._TieWithinEpsilon ? TEXT("true") : TEXT("false"));
                    }
                    LastEmittedAvoidanceSampleFrame = Trace._SampledFrame;
                }
            }

            ck::crowd::Display(TEXT("{} pipeline apply=({:.1f},{:.1f},{:.1f}) push=({:.1f},{:.1f},{:.1f}) pending_before_constrain=({:.1f},{:.1f},{:.1f}) observed_pos=({:.1f},{:.1f},{:.1f}) observed_delta=({:.1f},{:.1f},{:.1f})"),
                Prefix,
                P._PostApplyOffsetDisplacement.X, P._PostApplyOffsetDisplacement.Y, P._PostApplyOffsetDisplacement.Z,
                P._PushApartContribution.X, P._PushApartContribution.Y, P._PushApartContribution.Z,
                P._PendingDisplacementBeforeConstrain.X, P._PendingDisplacementBeforeConstrain.Y, P._PendingDisplacementBeforeConstrain.Z,
                P._PostConstrainObservedPosition.X, P._PostConstrainObservedPosition.Y, P._PostConstrainObservedPosition.Z,
                P._AppliedDisplacementSincePriorObservation.X, P._AppliedDisplacementSincePriorObservation.Y, P._AppliedDisplacementSincePriorObservation.Z);
        }
    }

    // RDP-simplified path. Always keep first + last; recursion fills interior keypoints.
    if (Samples.Num() == 0)
    { return; }

    auto Keep = TArray<bool>{};
    Keep.SetNumZeroed(Samples.Num());
    Keep[0] = true;
    Keep[Samples.Num() - 1] = true;

    if (Samples.Num() >= 3)
    {
        const auto Epsilon = FMath::Max(0.1f, ck_crowd_agent_diag_utils::CVarRDPEpsilon.GetValueOnGameThread());
        ck_crowd_agent_diag_utils::RDP_Recursive(Samples, 0, Samples.Num() - 1, Epsilon, Keep);
    }

    for (auto i = 0; i < Samples.Num(); ++i)
    {
        if (NOT Keep[i])
        { continue; }
        const auto& S = Samples[i];
        ck::crowd::Display(TEXT("{} simplified_path: t={:.2f} x={:.1f} y={:.1f} z={:.1f} v={}"),
            Prefix, S.Get_T(), S.Get_Pos().X, S.Get_Pos().Y, S.Get_Pos().Z, static_cast<int32>(S.Get_Speed()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
