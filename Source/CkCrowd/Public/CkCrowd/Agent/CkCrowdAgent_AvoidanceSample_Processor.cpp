#include "CkCrowdAgent_AvoidanceSample_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Settled_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AvoidanceSample);

DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample"), STAT_CkCrowd_AvoidanceSampleProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample (scan)"), STAT_CkCrowd_AvoidanceSample_Scan, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample (boundary refresh)"), STAT_CkCrowd_AvoidanceSample_BoundaryRefresh, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Agents Sampled"), STAT_CkCrowd_AgentsSampled, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_avoidance_sample
{
    // dtCrowd re-queries the local boundary once the agent has left a quarter of its collision
    // query range (DetourCrowd.cpp:1284).
    constexpr auto BOUNDARY_REFRESH_RANGE_FRACTION = 0.25f;

    // dtLocalBoundary::update's [UE] height filter (DetourLocalBoundary.cpp:88-95). The wall search
    // floods through CONNECTED polys, so a separate floor never reaches it; this is what keeps a
    // wall at the far end of a ramp or a step - reachable, but well above or below the agent's feet -
    // out of a planar solver that would otherwise treat it as a wall right here.
    constexpr auto BOUNDARY_MAX_HEIGHT_DIFFERENCE = 50.0f;

    // Upper bound on cache staleness for an agent that is NOT travelling: the navmesh under a
    // pinned or queued agent can be repainted (fixture UNavArea_Null boxes, stationary-markup tile
    // rebuilds) without it moving a quarter of its query range. dtLocalBoundary catches that through
    // poly-ref validation; FindEdges returns no refs, so a time cap stands in for it.
    constexpr auto BOUNDARY_MAX_CACHE_AGE_SEC = 1.0f;

    // Beyond roughly four rings a settled body near the path is indistinguishable from an unrelated
    // parked stranger, and predictive avoidance must stay ON when the agent is merely passing one;
    // the ceiling bounds how far the suppression can reach.
    constexpr auto PackExtentCeilingCm = 400.0f;

    // How far out from the destination the settled pack standing on it already reaches. Only bodies
    // that will not move out of the way count, and only within the ceiling above, so an agent with
    // no settled neighbours measures 0 and the envelope is exactly what it always was.
    auto Get_SettledPackExtentCm(
        const FVector& InAgentLocation,
        const FVector& InFinalWaypoint,
        bool InMarkupAnchorsEnabled,
        const ck::FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> float
    {
        auto PackExtentCm = 0.0f;

        for (const auto& Neighbour : InNeighborCache.Get_Neighbors())
        {
            const auto NeighbourAgent = UCk_Utils_CrowdAgent_UE::Cast(Neighbour.Get_Handle());
            if (ck::Is_NOT_Valid(NeighbourAgent))
            { continue; }

            if (NOT ck::ck_crowd_agent_settled_algorithm::Is_NeighbourSettled(
                    NeighbourAgent, InMarkupAnchorsEnabled))
            { continue; }

            // Planar, because the bodies are cylinders — the same metric the cluster detector's
            // goal-region test uses, so the two tiers measure one pack the same way.
            const auto NeighbourCentre = InAgentLocation + Neighbour.Get_RelativeOffset();
            const auto NeighbourDistanceToFinal =
                static_cast<float>(FVector::Dist2D(NeighbourCentre, InFinalWaypoint));

            if (NeighbourDistanceToFinal > PackExtentCeilingCm)
            { continue; }

            PackExtentCm = FMath::Max(PackExtentCm, NeighbourDistanceToFinal);
        }

        return PackExtentCm;
    }

    // True while the agent is on its LAST path leg and inside the final-approach envelope. Mirrors
    // FProcessor_CrowdAgent_Steering's arrival test exactly — same targeting predicate, same 3D
    // metric — so the envelope is a strict superset of the arrival condition it exists to let the
    // agent satisfy. A missing path/follow fragment means "not in the envelope", so the sampler
    // keeps its old behaviour rather than being silently switched off.
    //
    // The envelope grows by the settled pack's own extent because a fixed one is sized for an EMPTY
    // destination. Once bodies have settled around it, the agent's last leg ends at the pack's rim,
    // not at the goal, and the sampler's ordinary (non-overlap) time-to-impact term out-votes the
    // desired-velocity term there — a standoff several body-lengths out, which is OUTSIDE a fixed
    // envelope and therefore never suppressed. The cost function is right; it simply has no notion
    // that the bodies ahead will never move. Standing down at the pack's boundary is what lets
    // contact happen at all, and contact is the evidence BlockDetect's cluster tier needs to fire.
    auto Is_InFinalApproachEnvelope(
        const FCk_Handle_CrowdAgent& InAgent,
        const FVector& InAgentLocation,
        float InSuppressionCm,
        bool InMarkupAnchorsEnabled,
        const ck::FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> bool
    {
        if (NOT InAgent.Has<ck::FFragment_CrowdAgent_PathFollow>() ||
            NOT InAgent.Has<ck::FFragment_Nav_PathResult>())
        { return false; }

        const auto& Waypoints = InAgent.Get<ck::FFragment_Nav_PathResult>().Get_Waypoints();
        if (Waypoints.IsEmpty())
        { return false; }

        const auto& PathFollow = InAgent.Get<ck::FFragment_CrowdAgent_PathFollow>();
        const auto IsTargetingFinal = PathFollow.Get_WaypointIndex() == Waypoints.Num() - 1;
        if (NOT IsTargetingFinal)
        { return false; }

        const auto PackExtentCm = Get_SettledPackExtentCm(
            InAgentLocation, Waypoints.Last(), InMarkupAnchorsEnabled, InNeighborCache);

        const auto Envelope = PathFollow.Get_ActiveArrivalRadius() + InSuppressionCm + PackExtentCm;
        return FVector::Dist(InAgentLocation, Waypoints.Last()) <= Envelope;
    }

    // An explicit per-agent instruction outranks the envelope: both of these are deliberate caller
    // overrides that say "always sample this agent", not defaults for the envelope to second-guess.
    auto Has_ExplicitAlwaysSampleOverride(
        const FCk_Handle_CrowdAgent& InAgent) -> bool
    {
        if (InAgent.Has<ck::FFragment_CrowdAgent_AvoidancePolicy>() &&
            InAgent.Get<ck::FFragment_CrowdAgent_AvoidancePolicy>().Get_Policy() ==
                ECk_AvoidancePolicy::SamplingAlways)
        { return true; }

        return ck::ck_crowd_agent_avoidance_sample_algorithm::HasAvoidanceTag(
            InAgent, TAG_CrowdAvoidance_AlwaysSample);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_AvoidanceSample::
        DoRefreshLocalBoundary(
            const FCk_Handle& InAgent,
            const FVector& InAgentLocation,
            const float InQueryRange,
            const FVector& InProjectionExtent,
            FFragment_CrowdAgent_LocalBoundary& InOutBoundary)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSample_BoundaryRefresh);

        InOutBoundary._Centre = InAgentLocation;
        InOutBoundary._Segments.Reset();
        InOutBoundary._Valid = false;
        InOutBoundary._SecondsSinceRefresh = 0.0f;

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAgent);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto NavSys = UNavigationSystemV1::GetCurrent(World);
        auto* NavMesh = ck::IsValid(NavSys)
            ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;

        // A world with no Recast nav data has no walls to avoid - legitimate absence, exactly as
        // FProcessor_CrowdAgent_ConstrainToNavmesh treats it.
        if (ck::Is_NOT_Valid(NavMesh))
        { return; }

        const auto AgentPoly = NavMesh->FindNearestPoly(InAgentLocation, InProjectionExtent);
        if (AgentPoly == INVALID_NAVNODEREF)
        { return; }

        // findWallsInNeighbourhood is the local-neighbourhood flood of dtLocalBoundary::update fused
        // with its per-poly getPolyWallSegments, already converted back to Unreal space. A null
        // filter resolves to the navmesh's own default query filter.
        auto Edges = TArray<FNavigationWallEdge>{};
        if (NOT NavMesh->FindEdges(AgentPoly, InAgentLocation, InQueryRange, nullptr, Edges))
        { return; }

        InOutBoundary._Valid = true;

        using ck_crowd_agent_avoidance_sample_algorithm::DistancePointSegmentSquared2D;

        const auto RangeSquared = static_cast<double>(InQueryRange) * static_cast<double>(InQueryRange);
        auto Ranked = TArray<TPair<double, int32>, TInlineAllocator<16>>{};
        for (auto EdgeIndex = 0; EdgeIndex < Edges.Num(); ++EdgeIndex)
        {
            const auto& Edge = Edges[EdgeIndex];

            auto ClosestSegmentTime = 0.0;
            const auto DistanceSquared =
                DistancePointSegmentSquared2D(InAgentLocation, Edge.Start, Edge.End, &ClosestSegmentTime);
            if (DistanceSquared > RangeSquared)
            { continue; }

            const auto ClosestHeight = Edge.Start.Z + ((Edge.End.Z - Edge.Start.Z) * ClosestSegmentTime);
            if (FMath::Abs(ClosestHeight - InAgentLocation.Z) >
                ck_crowd_agent_avoidance_sample::BOUNDARY_MAX_HEIGHT_DIFFERENCE)
            { continue; }

            Ranked.Emplace(DistanceSquared, EdgeIndex);
        }

        // dtLocalBoundary::addSegment keeps the MAX_LOCAL_SEGS nearest, ordered by distance.
        Ranked.Sort([](const TPair<double, int32>& InLhs, const TPair<double, int32>& InRhs)
        {
            return InLhs.Key < InRhs.Key;
        });

        const auto KeptCount = FMath::Min(Ranked.Num(), FFragment_CrowdAgent_LocalBoundary::MaxSegments);
        for (auto RankIndex = 0; RankIndex < KeptCount; ++RankIndex)
        {
            const auto& Edge = Edges[Ranked[RankIndex].Value];
            InOutBoundary._Segments.Emplace(
                FFragment_CrowdAgent_LocalBoundary::FSegment{Edge.Start, Edge.End});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto FProcessor_CrowdAgent_AvoidanceSample::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Transform& InTransform,
        const FFragment_CrowdAgent_Params& InParams,
        const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
        FFragment_CrowdAgent_DesiredVelocity& InDesired,
        FFragment_CrowdAgent_LocalBoundary& InBoundary) const -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSampleProc);

        const auto SelfAgent = UCk_Utils_CrowdAgent_UE::Cast(ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity));
        if (NOT ck_crowd_agent_avoidance_sample_algorithm::ShouldRunSampling(SelfAgent, InNeighborCache))
        { return; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto AgentLocation = InTransform.Get_Transform().GetLocation();

        // ---- Final-approach suppression ----------------------------------------------------------
        // The standard Detour-caller idiom, and it breaks a hard lock-out rather than tuning one:
        // ringed by bodies at contact distance, every inward candidate closes on a near-overlapping
        // neighbour, the overlap branch of TimeToCollision floods ToiPen across the WHOLE cloud, and
        // the cheapest candidate is a standoff wider than the arrival radius. The agent then hovers
        // just outside its goal forever and never arrives — measured at 39-40cm against 30.
        //
        // The envelope's job is to stand down at the SETTLED PACK'S boundary, not at a fixed radius
        // around an empty goal: the sampler standoff is what starves BlockDetect's contact test, so
        // an agent held off the rim can never produce the contact its cluster tier blocks on.
        // Leaving Steering's path-follow velocity untouched is what lets it close the last stretch;
        // separation (lateral-clamped) and push-apart still run, so contact inside the envelope is
        // resolved by de-penetration, which is the correct authority a metre from the destination.
        const auto SuppressionCm = Settings->Get_AvoidanceFinalApproachSuppressionCm();
        const auto MarkupAnchorsEnabled =
            Settings->Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Enabled;

        if (SuppressionCm > 0.0f &&
            NOT ck_crowd_agent_avoidance_sample::Has_ExplicitAlwaysSampleOverride(SelfAgent) &&
            ck_crowd_agent_avoidance_sample::Is_InFinalApproachEnvelope(
                SelfAgent, AgentLocation, SuppressionCm, MarkupAnchorsEnabled, InNeighborCache))
        { return; }

        INC_DWORD_STAT(STAT_CkCrowd_AgentsSampled);
        if (InNeighborCache.Get_Neighbors().Num() == 0)
        { return; }

        auto Walls = ck_crowd_agent_avoidance_sample_algorithm::FWallSegments{};
        if (Settings->Get_AvoidanceWallSegments() == ECk_AvoidanceWallSegmentsMode::Enabled)
        {
            const auto QueryRange = InParams.Get_Radius() * Settings->Get_AvoidanceWallQueryRangeMultiplier();
            const auto RefreshDistance =
                QueryRange * ck_crowd_agent_avoidance_sample::BOUNDARY_REFRESH_RANGE_FRACTION;

            InBoundary._SecondsSinceRefresh += static_cast<float>(InDeltaT.Get_Seconds());

            if (NOT InBoundary.Get_Valid() ||
                InBoundary.Get_SecondsSinceRefresh() > ck_crowd_agent_avoidance_sample::BOUNDARY_MAX_CACHE_AGE_SEC ||
                FVector::DistSquared2D(AgentLocation, InBoundary.Get_Centre()) > FMath::Square(RefreshDistance))
            {
                const auto ProjectionExtent =
                    FVector{InParams.Get_Radius(), InParams.Get_Radius(), InParams.Get_Height()};
                DoRefreshLocalBoundary(SelfAgent, AgentLocation, QueryRange, ProjectionExtent, InBoundary);
            }

            Walls = ck_crowd_agent_avoidance_sample_algorithm::BuildWallSegments(AgentLocation, InBoundary);
        }

        const auto DesiredVelocity = InDesired.Get_Velocity();
        const auto Velocity = UCk_Utils_Velocity_UE::Cast(SelfAgent);
        const auto CurrentVelocity = ck::IsValid(Velocity) ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(Velocity) : FVector::ZeroVector;
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

        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSample_Scan);
        const auto Winner = ck_crowd_agent_avoidance_sample_algorithm::SelectWinner(Cloud, DesiredVelocity, CurrentVelocity, InNeighborCache, Parameters);
        if (Cloud._Candidates.IsValidIndex(Winner._CandidateIndex))
        { InDesired._Velocity = Cloud._Candidates[Winner._CandidateIndex]; }
    }
}

// --------------------------------------------------------------------------------------------------------------------
