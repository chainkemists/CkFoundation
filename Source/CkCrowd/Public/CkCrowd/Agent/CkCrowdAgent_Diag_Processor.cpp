#include "CkCrowdAgent_Diag_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagSensorTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagSeparationTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagSteeringTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagAvoidanceTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagAvoidanceScoreTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagAccelTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagVelocityBridgeTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagApplyOffsetTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagPushApartTap);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagRecorder);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DiagRecorder"), STAT_CkCrowd_DiagRecorderProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_processor
{
    // This CVar is consulted only by FTag_CrowdDiag_Tracked views. Raising its default has no
    // normal gameplay cost or behavioral effect.
    static TAutoConsoleVariable<int32> CVarSampleHz(
        TEXT("ck.Crowd.SampleHz"),
        20,
        TEXT("Sample rate (Hz) for tracked CrowdDiag pipeline/path samples.\n")
        TEXT("Higher = more granular diagnostics and memory per tracked agent. Default 20Hz."),
        ECVF_Cheat);

    static TAutoConsoleVariable<int32> CVarMaximumRecordedSamples(
        TEXT("ck.Crowd.MaxRecordedSamples"),
        ck::crowd_diag_breadcrumb::DefaultMaximumRecordedSamples,
        TEXT("Maximum retained raw CrowdDiag samples per tracked agent. History trims in batches.\n")
        TEXT("Minimum 256; default 4096."),
        ECVF_Cheat);

    constexpr auto ReversalAngleDeg = 90.0f;
    constexpr auto LoopMinSpeedCm = 20.0f;
    constexpr auto LoopMinSamples = 20;
    constexpr auto LoopMinSeconds = 1.0f;
    constexpr auto LoopMinTurnDeg = 360.0f;
    constexpr auto LoopMaxReverseDeg = 45.0f;
    constexpr auto LoopPathFraction = 0.8f;
    constexpr auto MaximumSpatialLoopRadii = 512;
    constexpr auto SpatialLoopRadiiTrimCount = 128;

    auto MedianRadius(const TArray<float>& InRadii) -> float
    {
        if (InRadii.Num() == 0)
        { return 0.0f; }

        auto Sorted = InRadii;
        Sorted.Sort();
        const auto Middle = Sorted.Num() / 2;
        return (Sorted.Num() % 2 == 0)
            ? 0.5f * (Sorted[Middle - 1] + Sorted[Middle])
            : Sorted[Middle];
    }

    auto MakeDiagScore(
        const ck::ck_crowd_agent_avoidance_sample_algorithm::FCandidateScore& InScore)
        -> FCk_CrowdDiag_AvoidanceScore
    {
        return {
            InScore._DesiredVelocityPenalty,
            InScore._CurrentVelocityPenalty,
            InScore._TimeToImpactPenalty,
            InScore._SidePenalty,
            InScore._TotalPenalty};
    }

    auto MakeDiagTimeToImpactContributor(
        const ck::ck_crowd_agent_avoidance_sample_algorithm::FMinimumTimeToCollision& InMinimum,
        const ck::FFragment_CrowdAgent_NeighborCache& InCache)
        -> FCk_CrowdDiag_TimeToImpactContributor
    {
        const auto& Neighbors = InCache.Get_Neighbors();
        if (NOT Neighbors.IsValidIndex(InMinimum._NeighborIndex))
        { return {}; }

        const auto& Neighbor = Neighbors[InMinimum._NeighborIndex];
        return {
            true,
            InMinimum._NeighborIndex,
            GetTypeHash(Neighbor.Get_Handle()),
            InMinimum._Time,
            Neighbor.Get_RelativeOffset(),
            Neighbor.Get_RelativeVelocity()};
    }

    auto HeadingDeltaDegrees(const FVector& InFrom, const FVector& InTo) -> float
    {
        if (InFrom.IsNearlyZero() || InTo.IsNearlyZero())
        { return 0.0f; }

        const auto FromHeading = FMath::RadiansToDegrees(FMath::Atan2(InFrom.Y, InFrom.X));
        const auto ToHeading = FMath::RadiansToDegrees(FMath::Atan2(InTo.Y, InTo.X));
        return FMath::FindDeltaAngleDegrees(FromHeading, ToHeading);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto FProcessor_CrowdAgent_DiagAvoidanceScoreTap::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Transform& InTransform,
        const FFragment_CrowdAgent_Params& InParams,
        const FFragment_CrowdAgent_NeighborCache& InCache,
        const FFragment_CrowdAgent_AvoidanceVolumeCache& InAvoidanceVolumeCache,
        const FFragment_CrowdAgent_LocalBoundary& InBoundary,
        const FFragment_CrowdAgent_DesiredVelocity& InDesired,
        FFragment_CrowdAgent_DiagRecorder& InRecorder) const -> void
    {
        const auto SelfAgent = UCk_Utils_CrowdAgent_UE::Cast(
            ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity));
        if (NOT ck_crowd_agent_avoidance_sample_algorithm::ShouldRunSampling(
            SelfAgent, InCache, InAvoidanceVolumeCache))
        { return; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings) ||
            (InCache.Get_Neighbors().Num() == 0 && InAvoidanceVolumeCache.Get_Obstacles().Num() == 0))
        { return; }

        const auto AgentLocation = InTransform.Get_Transform().GetLocation();
        auto Walls = Settings->Get_AvoidanceWallSegments() == ECk_AvoidanceWallSegmentsMode::Enabled
            ? ck_crowd_agent_avoidance_sample_algorithm::BuildWallSegments(AgentLocation, InBoundary)
            : ck_crowd_agent_avoidance_sample_algorithm::FWallSegments{};

        auto DesiredVelocity = InDesired.Get_Velocity();
        auto VolumeWallBuild = ck_crowd_agent_avoidance_sample_algorithm::BuildAvoidanceVolumeWalls(
            AgentLocation, InParams.Get_Radius(), InAvoidanceVolumeCache.Get_Obstacles());
        Walls.Append(VolumeWallBuild._Walls);
        if (NOT VolumeWallBuild._EscapeDirection.IsNearlyZero())
        { DesiredVelocity = VolumeWallBuild._EscapeDirection * InParams.Get_MaxSpeed(); }
        const auto SelfVelocity = UCk_Utils_Velocity_UE::Cast(SelfAgent);
        const auto CurrentVelocity = ck::IsValid(SelfVelocity)
            ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(SelfVelocity)
            : FVector::ZeroVector;
        const auto Parameters = ck_crowd_agent_avoidance_sample_algorithm::FScoringParameters{
            InParams.Get_Radius(), InParams.Get_MaxSpeed(), Settings->Get_AvoidanceHorizonTime(),
            Settings->Get_AvoidanceWeightDesVel(), Settings->Get_AvoidanceWeightCurVel(),
            Settings->Get_AvoidanceWeightSide(), Settings->Get_AvoidanceWeightToi(),
            Settings->Get_AvoidanceSidePreference(),
            ck_crowd_agent_avoidance_sample_algorithm::MakeReachabilityParameters(
                Settings->Get_AccelClampMode(),
                InDesired.Get_LastVelocity(),
                InParams.Get_MaxAcceleration(),
                InParams.Get_MaxTurnRate(),
                static_cast<float>(InDeltaT.Get_Seconds())),
            ck_crowd_agent_avoidance_sample_algorithm::FWallParameters{AgentLocation, Walls}};
        auto Cloud = ck_crowd_agent_avoidance_sample_algorithm::BuildSampleCloud(
            DesiredVelocity, Parameters._MaxSpeed, Settings->Get_AvoidanceVelBias(),
            Settings->Get_AvoidanceSampleAngularDivs(), Settings->Get_AvoidanceSampleRings(),
            Settings->Get_AvoidanceSampleDepth());

        const auto SampledFrame = static_cast<int32>(GFrameCounter);
        auto PriorWinnerObserved = false;
        int32 PriorWinnerSourceFrame = INDEX_NONE;
        auto PriorWinnerVelocity = FVector::ZeroVector;
        const auto& PriorTrace = InRecorder._LatestAvoidanceSampleTrace;
        if (NOT PriorTrace._Depths.IsEmpty() &&
            ck_crowd_agent_avoidance_sample_algorithm::IsExpectedPreviousSamplingFrame(
                SampledFrame,
                PriorTrace._SampledFrame,
                Settings->Get_AvoidanceSampleStride()))
        {
            PriorWinnerObserved = true;
            PriorWinnerSourceFrame = PriorTrace._SampledFrame;
            PriorWinnerVelocity = PriorTrace._Depths.Last()._SelectedCandidateVelocity;
        }

        auto DepthResults = ck_crowd_agent_avoidance_sample_algorithm::FSampleDepthResults{};
        const auto Winner = ck_crowd_agent_avoidance_sample_algorithm::SelectWinner(
            Cloud, DesiredVelocity, CurrentVelocity, InCache, Parameters, nullptr, &DepthResults);
        if (NOT Cloud._Candidates.IsValidIndex(Winner._CandidateIndex) || DepthResults.IsEmpty())
        { return; }

        const auto& FinalDepthResult = DepthResults.Last();
        auto WinnerMinimumTimeToCollision =
            ck_crowd_agent_avoidance_sample_algorithm::FMinimumTimeToCollision{};
        ck_crowd_agent_avoidance_sample_algorithm::CalculateCandidateScore(
            FinalDepthResult._WinnerVelocity,
            DesiredVelocity,
            CurrentVelocity,
            InCache,
            Parameters,
            &WinnerMinimumTimeToCollision);

        auto LatestTrace = FCk_CrowdDiag_AvoidanceSampleTrace{};
        LatestTrace._SampledFrame = SampledFrame;
        LatestTrace._DesiredVelocity = DesiredVelocity;
        LatestTrace._CurrentVelocity = CurrentVelocity;
        LatestTrace._NeighborCount = InCache.Get_Neighbors().Num();
        LatestTrace._AgentRadius = Parameters._AgentRadius;
        LatestTrace._MaxSpeed = Parameters._MaxSpeed;
        LatestTrace._Horizon = Parameters._Horizon;
        LatestTrace._WeightDesiredVelocity = Parameters._WeightDesiredVelocity;
        LatestTrace._WeightCurrentVelocity = Parameters._WeightCurrentVelocity;
        LatestTrace._WeightTimeToImpact = Parameters._WeightTimeToImpact;
        LatestTrace._WeightSide = Parameters._WeightSide;
        LatestTrace._SidePreference = static_cast<uint8>(Parameters._SidePreference);
        LatestTrace._WinnerTimeToImpactContributor =
            ck_crowd_agent_diag_processor::MakeDiagTimeToImpactContributor(
                WinnerMinimumTimeToCollision,
                InCache);

        if (PriorWinnerObserved)
        {
            auto& PriorWinner = LatestTrace._PriorWinnerRescore;
            PriorWinner._Observed = true;
            PriorWinner._SourceSampledFrame = PriorWinnerSourceFrame;
            PriorWinner._AgeFrames = LatestTrace._SampledFrame - PriorWinnerSourceFrame;
            PriorWinner._RawVelocity = PriorWinnerVelocity;
            PriorWinner._ScoredVelocity =
                ck_crowd_agent_avoidance_sample_algorithm::ResolveScoredCandidateVelocity(
                    PriorWinnerVelocity,
                    Parameters);
            PriorWinner._HeadingDeltaToCurrentWinnerDeg =
                ck_crowd_agent_diag_processor::HeadingDeltaDegrees(
                    PriorWinnerVelocity,
                    FinalDepthResult._WinnerVelocity);

            auto PriorMinimumTimeToCollision =
                ck_crowd_agent_avoidance_sample_algorithm::FMinimumTimeToCollision{};
            const auto PriorScore =
                ck_crowd_agent_avoidance_sample_algorithm::CalculateCandidateScore(
                    PriorWinnerVelocity,
                    DesiredVelocity,
                    CurrentVelocity,
                    InCache,
                    Parameters,
                    &PriorMinimumTimeToCollision);
            PriorWinner._Score = ck_crowd_agent_diag_processor::MakeDiagScore(PriorScore);
            PriorWinner._MinusCurrentWinnerPenalty =
                PriorScore._TotalPenalty - FinalDepthResult._Winner._Score._TotalPenalty;
            PriorWinner._TimeToImpactContributor =
                ck_crowd_agent_diag_processor::MakeDiagTimeToImpactContributor(
                    PriorMinimumTimeToCollision,
                    InCache);

            int32 NearestCandidateIndex = INDEX_NONE;
            auto NearestCandidateDistanceSquared = TNumericLimits<float>::Max();
            for (int32 CandidateIndex = 0; CandidateIndex < Cloud._Candidates.Num(); ++CandidateIndex)
            {
                const auto DistanceSquared =
                    (Cloud._Candidates[CandidateIndex] - PriorWinnerVelocity).SizeSquared2D();
                if (DistanceSquared < NearestCandidateDistanceSquared)
                {
                    NearestCandidateIndex = CandidateIndex;
                    NearestCandidateDistanceSquared = DistanceSquared;
                }
            }

            if (Cloud._Candidates.IsValidIndex(NearestCandidateIndex))
            {
                PriorWinner._NearestFinalCandidateIndex = NearestCandidateIndex;
                PriorWinner._NearestFinalCandidateRawVelocity =
                    Cloud._Candidates[NearestCandidateIndex];
                PriorWinner._NearestFinalCandidateScoredVelocity =
                    ck_crowd_agent_avoidance_sample_algorithm::ResolveScoredCandidateVelocity(
                        PriorWinner._NearestFinalCandidateRawVelocity,
                        Parameters);
                PriorWinner._NearestFinalCandidateDistance =
                    FMath::Sqrt(NearestCandidateDistanceSquared);

                auto NearestMinimumTimeToCollision =
                    ck_crowd_agent_avoidance_sample_algorithm::FMinimumTimeToCollision{};
                const auto NearestScore =
                    ck_crowd_agent_avoidance_sample_algorithm::CalculateCandidateScore(
                        PriorWinner._NearestFinalCandidateRawVelocity,
                        DesiredVelocity,
                        CurrentVelocity,
                        InCache,
                        Parameters,
                        &NearestMinimumTimeToCollision);
                PriorWinner._NearestFinalCandidateScore =
                    ck_crowd_agent_diag_processor::MakeDiagScore(NearestScore);
                PriorWinner._NearestFinalCandidateMinusWinnerPenalty =
                    NearestScore._TotalPenalty - FinalDepthResult._Winner._Score._TotalPenalty;
                PriorWinner._NearestFinalCandidateTimeToImpactContributor =
                    ck_crowd_agent_diag_processor::MakeDiagTimeToImpactContributor(
                        NearestMinimumTimeToCollision,
                        InCache);
            }
        }

        LatestTrace._Depths.Reserve(DepthResults.Num());
        for (const auto& DepthResult : DepthResults)
        {
            auto TraceDepth = FCk_CrowdDiag_AvoidanceDepthResult{};
            TraceDepth._Depth = DepthResult._Depth;
            TraceDepth._Centre = DepthResult._Centre;
            TraceDepth._Radius = DepthResult._Radius;
            TraceDepth._SelectedCandidateIndex = DepthResult._Winner._CandidateIndex;
            TraceDepth._SelectedCandidateVelocity = DepthResult._WinnerVelocity;
            TraceDepth._SelectedScoredVelocity = DepthResult._WinnerScoredVelocity;
            TraceDepth._SelectedDesiredVelocityPenalty = DepthResult._Winner._Score._DesiredVelocityPenalty;
            TraceDepth._SelectedCurrentVelocityPenalty = DepthResult._Winner._Score._CurrentVelocityPenalty;
            TraceDepth._SelectedTimeToImpactPenalty = DepthResult._Winner._Score._TimeToImpactPenalty;
            TraceDepth._SelectedSidePenalty = DepthResult._Winner._Score._SidePenalty;
            TraceDepth._SelectedTotalPenalty = DepthResult._Winner._Score._TotalPenalty;
            TraceDepth._MirrorPresent = DepthResult._MirrorPresent;
            TraceDepth._MirrorCandidateIndex = DepthResult._MirrorCandidateIndex;
            TraceDepth._MirrorCandidateVelocity = DepthResult._MirrorVelocity;
            TraceDepth._MirrorScoredVelocity = DepthResult._MirrorScoredVelocity;
            TraceDepth._MirrorDesiredVelocityPenalty = DepthResult._MirrorScore._DesiredVelocityPenalty;
            TraceDepth._MirrorCurrentVelocityPenalty = DepthResult._MirrorScore._CurrentVelocityPenalty;
            TraceDepth._MirrorTimeToImpactPenalty = DepthResult._MirrorScore._TimeToImpactPenalty;
            TraceDepth._MirrorSidePenalty = DepthResult._MirrorScore._SidePenalty;
            TraceDepth._MirrorTotalPenalty = DepthResult._MirrorScore._TotalPenalty;
            TraceDepth._MirrorMinusSelectedPenalty = DepthResult._MirrorMinusWinnerPenalty;
            TraceDepth._TieEpsilon = DepthResult._TieEpsilon;
            TraceDepth._TieWithinEpsilon = DepthResult._TieWithinEpsilon;
            LatestTrace._Depths.Add(MoveTemp(TraceDepth));
        }
        InRecorder._LatestAvoidanceSampleTrace = MoveTemp(LatestTrace);

        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._AvoidanceScoreObserved = true;
        Pipeline._AvoidanceSelectedCandidateIndex = FinalDepthResult._Winner._CandidateIndex;
        Pipeline._AvoidanceSelectedCandidateVelocity = FinalDepthResult._WinnerVelocity;
        Pipeline._AvoidanceSelectedDesiredVelocityPenalty = FinalDepthResult._Winner._Score._DesiredVelocityPenalty;
        Pipeline._AvoidanceSelectedCurrentVelocityPenalty = FinalDepthResult._Winner._Score._CurrentVelocityPenalty;
        Pipeline._AvoidanceSelectedTimeToImpactPenalty = FinalDepthResult._Winner._Score._TimeToImpactPenalty;
        Pipeline._AvoidanceSelectedSidePenalty = FinalDepthResult._Winner._Score._SidePenalty;
        Pipeline._AvoidanceSelectedTotalPenalty = FinalDepthResult._Winner._Score._TotalPenalty;
        Pipeline._AvoidanceTieEpsilon = FinalDepthResult._TieEpsilon;

        if (NOT FinalDepthResult._MirrorPresent)
        { return; }

        Pipeline._AvoidanceMirrorPresent = true;
        Pipeline._AvoidanceMirrorCandidateIndex = FinalDepthResult._MirrorCandidateIndex;
        Pipeline._AvoidanceMirrorCandidateVelocity = FinalDepthResult._MirrorVelocity;
        Pipeline._AvoidanceMirrorDesiredVelocityPenalty = FinalDepthResult._MirrorScore._DesiredVelocityPenalty;
        Pipeline._AvoidanceMirrorCurrentVelocityPenalty = FinalDepthResult._MirrorScore._CurrentVelocityPenalty;
        Pipeline._AvoidanceMirrorTimeToImpactPenalty = FinalDepthResult._MirrorScore._TimeToImpactPenalty;
        Pipeline._AvoidanceMirrorSidePenalty = FinalDepthResult._MirrorScore._SidePenalty;
        Pipeline._AvoidanceMirrorTotalPenalty = FinalDepthResult._MirrorScore._TotalPenalty;
        Pipeline._AvoidanceMirrorMinusSelectedPenalty = FinalDepthResult._MirrorMinusWinnerPenalty;
        Pipeline._AvoidanceTieWithinEpsilon = FinalDepthResult._TieWithinEpsilon;
    }

    auto FProcessor_CrowdAgent_DiagSensorTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline = {};
        Pipeline._Frame = static_cast<int32>(GFrameCounter);

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        Pipeline._RetainedNeighborCount = Neighbors.Num();
        if (Neighbors.Num() == 0)
        { return; }

        const auto& Nearest = Neighbors[0];
        Pipeline._HasNearestNeighbor = true;
        // NeighborCache distance is centre-to-centre; negative overlap values do not exist here.
        Pipeline._NearestHandleHash = GetTypeHash(Nearest.Get_Handle());
        Pipeline._NearestCenterDistance = static_cast<float>(Nearest.Get_Distance());
        Pipeline._NearestRelativeOffset = Nearest.Get_RelativeOffset();
        Pipeline._NearestRelativeVelocity = Nearest.Get_RelativeVelocity();
    }

    auto FProcessor_CrowdAgent_DiagSeparationTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_SeparationForce& InSeparationForce,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        InRecorder._PipelineFrame._SeparationForce = InSeparationForce.Get_Force();
    }

    auto FProcessor_CrowdAgent_DiagSteeringTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_PathFollow& InPathFollow,
        const FFragment_Nav_PathResult& InPathResult,
        const FFragment_CrowdAgent_DesiredVelocity& InDesired,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._WaypointIndex = InPathFollow.Get_WaypointIndex();
        Pipeline._ActiveGoal = InPathFollow.Get_ActiveGoal();
        Pipeline._ActiveArrivalRadius = InPathFollow.Get_ActiveArrivalRadius();
        Pipeline._SteeringVelocity = InDesired.Get_Velocity();

        const auto& Waypoints = InPathResult.Get_Waypoints();
        if (Waypoints.IsValidIndex(Pipeline._WaypointIndex))
        {
            Pipeline._HasWaypointIntent = true;
            Pipeline._WaypointTarget = Waypoints[Pipeline._WaypointIndex];
        }
    }

    auto FProcessor_CrowdAgent_DiagAvoidanceTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_DesiredVelocity& InDesired,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._PostAvoidanceOutputObserved = true;
        Pipeline._PostAvoidanceOutputVelocity = InDesired.Get_Velocity();
        Pipeline._PostAvoidanceOutputChangedSteering =
            NOT Pipeline._PostAvoidanceOutputVelocity.Equals(Pipeline._SteeringVelocity, KINDA_SMALL_NUMBER);
    }

    auto FProcessor_CrowdAgent_DiagAccelTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_DesiredVelocity& InDesired,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._AccelInputVelocity = Pipeline._PostAvoidanceOutputObserved
            ? Pipeline._PostAvoidanceOutputVelocity
            : Pipeline._SteeringVelocity;
        Pipeline._AccelOutputVelocity = InDesired.Get_Velocity();
        Pipeline._AccelLastVelocityAfter = InDesired.Get_LastVelocity();
    }

    auto FProcessor_CrowdAgent_DiagVelocityBridgeTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_Velocity_Current& InCurrentVelocity,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        InRecorder._PipelineFrame._BridgedVelocity = InCurrentVelocity.Get_CurrentVelocity();
    }

    auto FProcessor_CrowdAgent_DiagApplyOffsetTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_PendingDisplacement& InPending,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        InRecorder._PipelineFrame._PostApplyOffsetDisplacement = InPending.Get_Displacement();
    }

    auto FProcessor_CrowdAgent_DiagPushApartTap::ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_CrowdAgent_PendingDisplacement& InPending,
        FFragment_CrowdAgent_DiagRecorder& InRecorder)
    -> void
    {
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._PendingDisplacementBeforeConstrain = InPending.Get_Displacement();
        Pipeline._PushApartContribution =
            Pipeline._PendingDisplacementBeforeConstrain - Pipeline._PostApplyOffsetDisplacement;
    }

    auto
        FProcessor_CrowdAgent_DiagRecorder::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Velocity_Current& InCurrentVelocity,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DiagRecorder& InRecorder)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DiagRecorderProc);

        const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
        InRecorder._ElapsedSec += Dt;
        InRecorder._SecsSinceLastSample += Dt;

        const auto Pos = InTransform.Get_Transform().GetLocation();
        auto& Pipeline = InRecorder._PipelineFrame;
        Pipeline._PostConstrainObservedPosition = Pos;
        Pipeline._AppliedDisplacementSincePriorObservation = InRecorder._HasLastObservedPostConstrainPosition
            ? Pos - InRecorder._LastObservedPostConstrainPosition
            : FVector::ZeroVector;
        Pipeline._ConstrainConsumedPendingDisplacement =
            NOT Pipeline._PendingDisplacementBeforeConstrain.IsNearlyZero();
        InRecorder._LastObservedPostConstrainPosition = Pos;
        InRecorder._HasLastObservedPostConstrainPosition = true;

        const auto SampleHz = FMath::Max(1, ck_crowd_agent_diag_processor::CVarSampleHz.GetValueOnGameThread());
        const auto SampleIntervalSec = 1.0f / static_cast<float>(SampleHz);
        if (InRecorder._SecsSinceLastSample < SampleIntervalSec)
        { return; }
        InRecorder._SecsSinceLastSample = 0.0f;

        const auto ActualVelocity = InCurrentVelocity.Get_CurrentVelocity();
        Pipeline._FinalCurrentVelocity = ActualVelocity;
        const auto Speed = static_cast<float>(ActualVelocity.Size());
        // Heading comes off the HORIZONTAL velocity only, so it must be gated on the
        // horizontal magnitude. Gating it on Speed (3D) let a purely vertical velocity --
        // every agent's spawn-frame fall -- pass as "moving" while atan2(0, 0) returned 0,
        // stamping a phantom +X heading on the first sample. The next sample's real heading
        // then read as a swing of exactly the agent's bearing off +X, so _MaxAngularDeltaDeg
        // measured spawn geometry instead of motion.
        const auto Speed2D = static_cast<float>(ActualVelocity.Size2D());
        const auto HasDir = Speed2D > KINDA_SMALL_NUMBER;
        const auto DirRad = HasDir
            ? static_cast<float>(FMath::Atan2(ActualVelocity.Y, ActualVelocity.X))
            : 0.0f;

        auto Sample = FCk_CrowdDiag_PathSample{};
        Sample._T = InRecorder._ElapsedSec;
        Sample._Pos = Pos;
        Sample._Speed = Speed;
        Sample._DirRad = DirRad;
        Sample._HasDir = HasDir;
        Sample._Pipeline = Pipeline;
        const auto RecorderFrame = static_cast<int32>(GFrameCounter);
        const auto& LatestAvoidanceTrace = InRecorder._LatestAvoidanceSampleTrace;
        if (LatestAvoidanceTrace._SampledFrame != INDEX_NONE)
        {
            if (InRecorder._AvoidanceSampleTraces.IsEmpty() ||
                InRecorder._AvoidanceSampleTraces.Last()._SampledFrame != LatestAvoidanceTrace._SampledFrame)
            {
                InRecorder._AvoidanceSampleTraces.Add(LatestAvoidanceTrace);
            }
            Sample._AvoidanceSampleTraceIndex = InRecorder._AvoidanceSampleTraces.Num() - 1;
            Sample._AvoidanceSampleTraceAgeFrames =
                FMath::Max(0, RecorderFrame - LatestAvoidanceTrace._SampledFrame);
        }
        const auto TrackGoalDistance = static_cast<float>(FVector::Dist2D(Pos, InRecorder._GoalPos));
        InRecorder._CurrentTrackGoalDistance = TrackGoalDistance;
        InRecorder._BestTrackGoalDistance = FMath::Min(InRecorder._BestTrackGoalDistance, TrackGoalDistance);
        Sample._TrackGoalDistance = TrackGoalDistance;
        Sample._BestTrackGoalDistance = InRecorder._BestTrackGoalDistance;

        const auto AppendSample = [&InRecorder](const FCk_CrowdDiag_PathSample& InSample)
        {
            InRecorder._Samples.Add(InSample);

            const auto ConfiguredMaximum =
                ck_crowd_agent_diag_processor::CVarMaximumRecordedSamples.GetValueOnGameThread();
            const auto RemoveCount = crowd_diag_breadcrumb::GetRecorderTrimCount(
                InRecorder._Samples.Num(),
                ConfiguredMaximum);
            if (RemoveCount == 0)
            { return; }

            InRecorder._RetainedHistoryStartPos = InRecorder._Samples[RemoveCount - 1].Get_Pos();
            InRecorder._Samples.RemoveAt(0, RemoveCount, EAllowShrinking::No);

            if (InRecorder._QualifiedSpatialLoopLastSampleIndex != INDEX_NONE)
            {
                InRecorder._QualifiedSpatialLoopLastSampleIndex -= RemoveCount;
                if (InRecorder._QualifiedSpatialLoopLastSampleIndex < 0)
                { InRecorder._QualifiedSpatialLoopLastSampleIndex = INDEX_NONE; }
            }

            auto FirstReferencedTrace = MAX_int32;
            for (auto& RetainedSample : InRecorder._Samples)
            {
                const auto TraceIndex = RetainedSample._AvoidanceSampleTraceIndex;
                if (TraceIndex == INDEX_NONE)
                { continue; }
                if (NOT InRecorder._AvoidanceSampleTraces.IsValidIndex(TraceIndex))
                {
                    RetainedSample._AvoidanceSampleTraceIndex = INDEX_NONE;
                    RetainedSample._AvoidanceSampleTraceAgeFrames = INDEX_NONE;
                    continue;
                }
                FirstReferencedTrace = FMath::Min(FirstReferencedTrace, TraceIndex);
            }

            if (FirstReferencedTrace == MAX_int32)
            {
                InRecorder._AvoidanceSampleTraces.Reset();
                return;
            }
            if (FirstReferencedTrace == 0)
            { return; }

            InRecorder._AvoidanceSampleTraces.RemoveAt(
                0,
                FirstReferencedTrace,
                EAllowShrinking::No);
            for (auto& RetainedSample : InRecorder._Samples)
            {
                if (RetainedSample._AvoidanceSampleTraceIndex != INDEX_NONE)
                { RetainedSample._AvoidanceSampleTraceIndex -= FirstReferencedTrace; }
            }
        };
        // NeighborCache is sorted nearest-first and stores centre-to-centre distance.
        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() > 0)
        {
            const auto NearestDist = static_cast<float>(Neighbors[0].Get_Distance());
            if (NearestDist < InRecorder._MinSepAcrossCycle)
            {
                InRecorder._MinSepAcrossCycle = NearestDist;
                InRecorder._MinSepTime = InRecorder._ElapsedSec;
            }
        }

        if (InRecorder._Samples.Num() >= 1)
        {
            const auto& Prev = InRecorder._Samples.Last();
            if (Prev._HasDir && HasDir)
            {
                auto DeltaRad = DirRad - Prev._DirRad;
                while (DeltaRad > PI) { DeltaRad -= 2.0f * PI; }
                while (DeltaRad < -PI) { DeltaRad += 2.0f * PI; }
                const auto DeltaDeg = FMath::Abs(FMath::RadiansToDegrees(DeltaRad));
                InRecorder._MaxAngularDeltaDeg = FMath::Max(InRecorder._MaxAngularDeltaDeg, DeltaDeg);
                if (DeltaDeg >= ck_crowd_agent_diag_processor::ReversalAngleDeg)
                { ++InRecorder._DirReversalCount; }
            }
        }

        if (NOT InRecorder._Reached)
        {
            // Path-following can supply a more specific active radius than the agent Params.
            const auto ActiveArrivalRadius = Pipeline._ActiveArrivalRadius > KINDA_SMALL_NUMBER
                ? Pipeline._ActiveArrivalRadius
                : InParams.Get_ArrivalRadius();
            if (TrackGoalDistance <= ActiveArrivalRadius)
            {
                InRecorder._Reached = true;
                InRecorder._TimeToGoal = InRecorder._ElapsedSec;
            }
        }

        const auto ToSpatialCenter = Pos - InRecorder._SpatialCenterPos;
        const auto Radius = static_cast<float>(ToSpatialCenter.Size2D());
        const auto ActiveArrivalRadius = Pipeline._ActiveArrivalRadius > KINDA_SMALL_NUMBER
            ? Pipeline._ActiveArrivalRadius
            : InParams.Get_ArrivalRadius();
        const auto MinLoopRadius = ActiveArrivalRadius + 10.0f;
        const auto IsLoopEligible = NOT InRecorder._Reached &&
            Speed >= ck_crowd_agent_diag_processor::LoopMinSpeedCm && Radius >= MinLoopRadius;
        Sample._SpatialLoopEligible = IsLoopEligible;
        if (NOT IsLoopEligible)
        {
            // An ineligible sample terminates the current evidence window. Never stitch an orbit
            // across a reach, idle, or radius gap; a previous qualified window stays latched.
            if (InRecorder._SpatialLoopWindowActive)
            {
                const auto ResetReason = InRecorder._Reached
                    ? ECk_CrowdDiag_SpatialWindowResetReason::Reached
                    : Speed < ck_crowd_agent_diag_processor::LoopMinSpeedCm
                    ? ECk_CrowdDiag_SpatialWindowResetReason::SpeedBelowMinimum
                    : ECk_CrowdDiag_SpatialWindowResetReason::RadiusBelowMinimum;
                const auto IsStrongerThanBestCompleted = NOT InRecorder._HasBestCompletedSpatialWindow ||
                    InRecorder._DominantSpatialTurnDeg > InRecorder._BestCompletedSpatialDominantTurnDeg + KINDA_SMALL_NUMBER ||
                    (FMath::IsNearlyEqual(InRecorder._DominantSpatialTurnDeg, InRecorder._BestCompletedSpatialDominantTurnDeg) &&
                        InRecorder._CumulativeAbsoluteSpatialTurnDeg > InRecorder._BestCompletedSpatialTurnAbsoluteDeg + KINDA_SMALL_NUMBER);
                if (NOT InRecorder._CurrentSpatialWindowQualified && IsStrongerThanBestCompleted)
                {
                    InRecorder._HasBestCompletedSpatialWindow = true;
                    InRecorder._BestCompletedSpatialWindowId = InRecorder._CurrentSpatialLoopWindowId;
                    InRecorder._BestCompletedSpatialTurnSignedDeg = InRecorder._CumulativeSignedSpatialTurnDeg;
                    InRecorder._BestCompletedSpatialTurnAbsoluteDeg = InRecorder._CumulativeAbsoluteSpatialTurnDeg;
                    InRecorder._BestCompletedSpatialDominantTurnDeg = InRecorder._DominantSpatialTurnDeg;
                    InRecorder._BestCompletedSpatialReverseTurnDeg = InRecorder._ReverseSpatialTurnDeg;
                    InRecorder._BestCompletedSpatialEligibleSamples = InRecorder._EligibleSpatialSamples;
                    InRecorder._BestCompletedSpatialElapsedSeconds =
                        InRecorder._LastEligibleSpatialTime - InRecorder._FirstEligibleSpatialTime;
                    InRecorder._BestCompletedSpatialPathLength = InRecorder._EligibleSpatialPathLength;
                    InRecorder._BestCompletedSpatialRepresentativeRadius = InRecorder._SpatialLoopRepresentativeRadius;
                    InRecorder._BestCompletedSpatialMinRadius = InRecorder._SpatialLoopMinRadius;
                    InRecorder._BestCompletedSpatialMaxRadius = InRecorder._SpatialLoopMaxRadius;
                    InRecorder._BestCompletedSpatialRequiredPathLength = InRecorder._SpatialLoopRequiredPathLength;
                    InRecorder._BestCompletedSpatialClosureDistance = InRecorder._SpatialLoopClosureDistance;
                    const auto CompletedWindowEndGoalDistance = InRecorder._Samples.IsEmpty()
                        ? TrackGoalDistance
                        : InRecorder._Samples.Last().Get_TrackGoalDistance();
                    InRecorder._BestCompletedSpatialWindowNetGoalProgress =
                        InRecorder._CurrentSpatialWindowStartGoalDistance - CompletedWindowEndGoalDistance;
                    InRecorder._BestCompletedSpatialResetReason = ResetReason;
                    InRecorder._BestCompletedSpatialResetSpeed = Speed;
                    InRecorder._BestCompletedSpatialResetMinimumSpeed = ck_crowd_agent_diag_processor::LoopMinSpeedCm;
                    InRecorder._BestCompletedSpatialResetRadius = Radius;
                    InRecorder._BestCompletedSpatialResetMinimumRadius = MinLoopRadius;
                }
                InRecorder._SpatialLoopWindowActive = false;
                InRecorder._CumulativeSignedSpatialTurnDeg = 0.0f;
                InRecorder._CumulativeAbsoluteSpatialTurnDeg = 0.0f;
                InRecorder._DominantSpatialTurnDeg = 0.0f;
                InRecorder._ReverseSpatialTurnDeg = 0.0f;
                InRecorder._EligibleSpatialSamples = 0;
                InRecorder._FirstEligibleSpatialTime = 0.0f;
                InRecorder._LastEligibleSpatialTime = 0.0f;
                InRecorder._LastEligibleSpatialAngleRad = 0.0f;
                InRecorder._HasLastEligibleSpatialAngle = false;
                InRecorder._EligibleSpatialRadii.Reset();
                InRecorder._EligibleSpatialPathLength = 0.0f;
                InRecorder._LastEligibleSpatialPosition = FVector::ZeroVector;
                InRecorder._HasLastEligibleSpatialPosition = false;
                InRecorder._FirstEligibleSpatialPosition = FVector::ZeroVector;
                InRecorder._HasFirstEligibleSpatialPosition = false;
                InRecorder._SpatialLoopClosureDistance = 0.0f;
                InRecorder._SpatialLoopRepresentativeRadius = 0.0f;
                InRecorder._SpatialLoopRequiredPathLength = 0.0f;
                InRecorder._SpatialLoopMinRadius = TNumericLimits<float>::Max();
                InRecorder._SpatialLoopMaxRadius = 0.0f;
                InRecorder._CurrentSpatialWindowStartGoalDistance = 0.0f;
                InRecorder._CurrentSpatialWindowQualified = false;
            }
            AppendSample(Sample);
            return;
        }

        if (NOT InRecorder._SpatialLoopWindowActive)
        {
            ++InRecorder._CurrentSpatialLoopWindowId;
            InRecorder._SpatialLoopWindowActive = true;
            InRecorder._CurrentSpatialWindowStartGoalDistance = TrackGoalDistance;
            InRecorder._CurrentSpatialWindowQualified = false;
        }
        Sample._SpatialLoopWindowId = InRecorder._CurrentSpatialLoopWindowId;

        const auto Angle = static_cast<float>(FMath::Atan2(ToSpatialCenter.Y, ToSpatialCenter.X));
        if (NOT InRecorder._HasFirstEligibleSpatialPosition)
        {
            InRecorder._FirstEligibleSpatialPosition = Pos;
            InRecorder._HasFirstEligibleSpatialPosition = true;
            InRecorder._FirstEligibleSpatialTime = InRecorder._ElapsedSec;
        }
        if (InRecorder._HasLastEligibleSpatialPosition)
        {
            InRecorder._EligibleSpatialPathLength += FVector::Dist2D(Pos, InRecorder._LastEligibleSpatialPosition);
        }
        InRecorder._LastEligibleSpatialPosition = Pos;
        InRecorder._HasLastEligibleSpatialPosition = true;
        InRecorder._EligibleSpatialRadii.Add(Radius);
        if (InRecorder._EligibleSpatialRadii.Num() > ck_crowd_agent_diag_processor::MaximumSpatialLoopRadii)
        {
            InRecorder._EligibleSpatialRadii.RemoveAt(
                0,
                ck_crowd_agent_diag_processor::SpatialLoopRadiiTrimCount,
                EAllowShrinking::No);
        }
        InRecorder._SpatialLoopMinRadius = FMath::Min(InRecorder._SpatialLoopMinRadius, Radius);
        InRecorder._SpatialLoopMaxRadius = FMath::Max(InRecorder._SpatialLoopMaxRadius, Radius);
        ++InRecorder._EligibleSpatialSamples;
        InRecorder._LastEligibleSpatialTime = InRecorder._ElapsedSec;

        if (InRecorder._HasLastEligibleSpatialAngle)
        {
            auto DeltaAngle = Angle - InRecorder._LastEligibleSpatialAngleRad;
            while (DeltaAngle > PI) { DeltaAngle -= 2.0f * PI; }
            while (DeltaAngle < -PI) { DeltaAngle += 2.0f * PI; }
            const auto DeltaDeg = FMath::RadiansToDegrees(DeltaAngle);
            InRecorder._CumulativeSignedSpatialTurnDeg += DeltaDeg;
            InRecorder._CumulativeAbsoluteSpatialTurnDeg += FMath::Abs(DeltaDeg);
            InRecorder._DominantSpatialTurnDeg = 0.5f * (
                InRecorder._CumulativeAbsoluteSpatialTurnDeg + FMath::Abs(InRecorder._CumulativeSignedSpatialTurnDeg));
            InRecorder._ReverseSpatialTurnDeg = 0.5f * (
                InRecorder._CumulativeAbsoluteSpatialTurnDeg - FMath::Abs(InRecorder._CumulativeSignedSpatialTurnDeg));
        }
        InRecorder._LastEligibleSpatialAngleRad = Angle;
        InRecorder._HasLastEligibleSpatialAngle = true;

        if (InRecorder._HasFirstEligibleSpatialPosition)
        {
            InRecorder._SpatialLoopClosureDistance = FVector::Dist2D(Pos, InRecorder._FirstEligibleSpatialPosition);
        }

        const auto MedianRadius = ck_crowd_agent_diag_processor::MedianRadius(InRecorder._EligibleSpatialRadii);
        const auto RequiredPath = 2.0f * PI * MedianRadius * ck_crowd_agent_diag_processor::LoopPathFraction;
        InRecorder._SpatialLoopRepresentativeRadius = MedianRadius;
        InRecorder._SpatialLoopRequiredPathLength = RequiredPath;
        const auto EligibleSeconds = InRecorder._ElapsedSec - InRecorder._FirstEligibleSpatialTime;
        const auto MeaningfulRadiusBand = MedianRadius >= MinLoopRadius &&
            (InRecorder._SpatialLoopMaxRadius - InRecorder._SpatialLoopMinRadius) <= MedianRadius;
        const auto IsClosed = InRecorder._SpatialLoopClosureDistance <= FMath::Max(20.0f, MedianRadius * 0.5f);
        const auto IsQualifiedWindow = NOT InRecorder._Reached &&
            InRecorder._EligibleSpatialSamples >= ck_crowd_agent_diag_processor::LoopMinSamples &&
            EligibleSeconds >= ck_crowd_agent_diag_processor::LoopMinSeconds &&
            InRecorder._DominantSpatialTurnDeg >= ck_crowd_agent_diag_processor::LoopMinTurnDeg &&
            InRecorder._ReverseSpatialTurnDeg < ck_crowd_agent_diag_processor::LoopMaxReverseDeg &&
            MeaningfulRadiusBand && IsClosed && InRecorder._EligibleSpatialPathLength >= RequiredPath;
        InRecorder._CurrentSpatialWindowQualified = InRecorder._CurrentSpatialWindowQualified || IsQualifiedWindow;

        // Preserve the first complete, uninterrupted orbit. Digest rows are filtered by this
        // window id, so later motion cannot mix into or erase the captured evidence.
        if (NOT InRecorder._SpatialLoopQualified && IsQualifiedWindow)
        {
            InRecorder._SpatialLoopQualified = true;
            InRecorder._QualifiedSpatialLoopWindowId = InRecorder._CurrentSpatialLoopWindowId;
            // The current sample is appended below. Freeze the qualified prefix rather than
            // allowing later eligible motion in the same live window to rewrite its evidence.
            InRecorder._QualifiedSpatialLoopLastSampleIndex = InRecorder._Samples.Num();
            InRecorder._QualifiedSpatialTurnSignedDeg = InRecorder._CumulativeSignedSpatialTurnDeg;
            InRecorder._QualifiedSpatialTurnAbsoluteDeg = InRecorder._CumulativeAbsoluteSpatialTurnDeg;
            InRecorder._QualifiedSpatialDominantTurnDeg = InRecorder._DominantSpatialTurnDeg;
            InRecorder._QualifiedSpatialReverseTurnDeg = InRecorder._ReverseSpatialTurnDeg;
            InRecorder._QualifiedSpatialEligibleSamples = InRecorder._EligibleSpatialSamples;
            InRecorder._QualifiedSpatialPathLength = InRecorder._EligibleSpatialPathLength;
            InRecorder._QualifiedSpatialRepresentativeRadius = InRecorder._SpatialLoopRepresentativeRadius;
            InRecorder._QualifiedSpatialRequiredPathLength = InRecorder._SpatialLoopRequiredPathLength;
            InRecorder._QualifiedSpatialClosureDistance = InRecorder._SpatialLoopClosureDistance;
            InRecorder._QualifiedSpatialWindowNetGoalProgress =
                InRecorder._CurrentSpatialWindowStartGoalDistance - TrackGoalDistance;
        }
        AppendSample(Sample);
    }
}

// --------------------------------------------------------------------------------------------------------------------
