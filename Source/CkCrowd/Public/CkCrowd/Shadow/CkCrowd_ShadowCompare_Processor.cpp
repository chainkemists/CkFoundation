#include "CkCrowd_ShadowCompare_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkCrowd/CkCrowd_Log.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_GroundNavInstall_Algorithm.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Fragment.h"
#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Report.h"
#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNav_ShadowCompare);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::GroundNavShadowCompare"), STAT_CkCrowd_GroundNavShadowCompareProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_shadow_compare_processor
{
    auto Get_IsTerminal(
        ECk_Nav_PathStatus InStatus) -> bool
    {
        return InStatus == ECk_Nav_PathStatus::Ready
            || InStatus == ECk_Nav_PathStatus::Partial
            || InStatus == ECk_Nav_PathStatus::Failed;
    }

    // A waypoint sitting on the agent is the plan's own start point, not a corner of the route.
    // GroundNav keeps it and CkNavigation strips it, so it has to be recognised rather than counted.
    inline constexpr double ShadowStartPointEpsilonUu = 1.0;

    /** Measured from where the agent actually stands, not from the first waypoint. CkNavigation
     *  strips the start point, so a straight route installs as a SINGLE waypoint and a sum over the
     *  array alone reads zero against a GroundNav plan that kept its start - the whole route as a
     *  delta, on a pair that did not diverge at all. */
    auto Get_PolylineLengthUu(
        const FVector&         InAgentLocation,
        const TArray<FVector>& InWaypoints) -> double
    {
        auto Length = 0.0;
        auto Previous = InAgentLocation;

        for (const auto& Waypoint : InWaypoints)
        {
            Length += FVector::Dist(Previous, Waypoint);
            Previous = Waypoint;
        }

        return Length;
    }

    /** The corners of the route, with a leading start point discounted on whichever side kept one -
     *  otherwise the count delta reports a difference in convention rather than in the route. */
    auto Get_EffectiveWaypointCount(
        const FVector&         InAgentLocation,
        const TArray<FVector>& InWaypoints) -> int32
    {
        if (InWaypoints.IsEmpty())
        { return 0; }

        const auto LeadsWithStartPoint =
            FVector::Dist(InAgentLocation, InWaypoints[0]) <= ShadowStartPointEpsilonUu;

        return LeadsWithStartPoint ? InWaypoints.Num() - 1 : InWaypoints.Num();
    }

    /** A route that produced no waypoints ended nowhere of its own, so the query's goal stands in for
     *  it. Both halves fall back to the SAME goal, which is what makes a both-failed pair contribute a
     *  zero endpoint delta rather than a distance to the origin. */
    auto Get_Endpoint(
        const TArray<FVector>& InWaypoints,
        const FVector&         InFallback) -> FVector
    {
        return InWaypoints.IsEmpty() ? InFallback : InWaypoints.Last();
    }

    /**
     * A name two runs of the same fixture produce identically.
     *
     * An agent's debug name is its gameplay label when it has one - CkLabel stamps the tag name as the
     * debug name on Add - and otherwise degrades to the handle string, which carries the entity id and
     * so differs run to run. Only the label branch is taken; an unlabelled agent buckets under the
     * fixture, which makes its id ambiguous across agents — only a labelled agent gets a name that
     * identifies one comparison.
     */
    auto Get_QueryId(
        const FCk_Handle& InHandle,
        FName             InFixtureKey,
        int32             InRevision) -> FName
    {
        const auto AgentIsLabelled = UCk_Utils_GameplayLabel_UE::Has(InHandle)
            && NOT UCk_Utils_GameplayLabel_UE::Get_IsUnnamedLabel(InHandle);

        const auto AgentKey = AgentIsLabelled
            ? UCk_Utils_GameplayLabel_UE::Get_Label(InHandle).GetTagName()
            : InFixtureKey;

        return FName{*FString::Printf(TEXT("%s#rev%d"), *AgentKey.ToString(), InRevision)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_GroundNav_ShadowCompare::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_Nav_PathResult& InNavResult,
            const FFragment_GroundNavPath_Result& InGroundNavResult)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_GroundNavShadowCompareProc);

        const auto& GroundNavResult = InGroundNavResult.Get_Result();

        if (GroundNavResult.Get_IsShadow() != ECk_EnableDisable::Enable)
        { return; }

        if (NOT InGroundNavResult.Get_HasFreshResult())
        { return; }

        // The shadow query was issued carrying the Recast query's own revision, so equality is what
        // says the two slots answer the SAME MoveTo rather than two episodes that happen to overlap.
        const auto Revision = GroundNavResult.Get_RequestRevision();

        if (Revision != InNavResult.Get_RequestRevision())
        { return; }

        if (NOT ck_crowd_shadow_compare_processor::Get_IsTerminal(InNavResult.Get_Status()))
        { return; }

        if (InHandle.Has<FFragment_CrowdAgent_ShadowCompared>() &&
            InHandle.Get<FFragment_CrowdAgent_ShadowCompared>().Get_LastComparedRevision() == Revision)
        { return; }

        const auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto WorldIsResolvable = ck::IsValid(World);

        CK_ENSURE_IF_NOT(WorldIsResolvable,
            TEXT("CrowdAgent [{}] holds a shadow GroundNav result but resolves no world - the "
                 "comparison has no fixture to bucket under and is dropped"), InHandle)
        { return; }

        const auto FallbackKey = UCk_Utils_GroundNav_Shadow_UE::Get_FallbackFixtureKey(World);

        auto WorldEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle);
        auto& Diagnostics = WorldEntity.AddOrGet<FFragment_GroundNav_ShadowDiagnostics>();

        const auto FixtureKey = Diagnostics.Get_ActiveFixture().IsNone()
            ? FallbackKey
            : Diagnostics.Get_ActiveFixture();

        const auto& RecastWaypoints = InNavResult.Get_Waypoints();
        const auto& GroundNavWaypoints = GroundNavResult.Get_Waypoints();
        const auto& RecastDiagnostics = InNavResult.Get_Diagnostics();
        const auto Goal = InNavResult.Get_DestinationLocation();
        const auto AgentLocation = InTransform.Get_Transform().GetLocation();

        // The two providers do not share a fail vocabulary, and whether they agree about a reason is
        // only answerable once they do - the install table is where that mapping already lives.
        const auto& Verdict = ck_crowd_agent_ground_nav_install_algorithm::Get_GroundNavVerdict(
            GroundNavResult.Get_Status());

        auto Comparison = groundnav::FCk_GroundNav_ShadowComparison{};

        Comparison._QueryId = ck_crowd_shadow_compare_processor::Get_QueryId(InHandle, FixtureKey, Revision);

        Comparison._RecastStatus = InNavResult.Get_Status();
        Comparison._RecastFailReason = RecastDiagnostics.Get_LastFailReason();
        Comparison._RecastWaypointCount = ck_crowd_shadow_compare_processor::Get_EffectiveWaypointCount(AgentLocation, RecastWaypoints);
        Comparison._RecastLengthUu = ck_crowd_shadow_compare_processor::Get_PolylineLengthUu(AgentLocation, RecastWaypoints);
        Comparison._RecastEndpoint = ck_crowd_shadow_compare_processor::Get_Endpoint(RecastWaypoints, Goal);
        Comparison._RecastQueryMs = RecastDiagnostics.Get_LastQueryDurationMs();

        Comparison._GroundNavStatus = GroundNavResult.Get_Status();
        Comparison._GroundNavFailReason = Verdict._Reason;
        Comparison._GroundNavWaypointCount = ck_crowd_shadow_compare_processor::Get_EffectiveWaypointCount(AgentLocation, GroundNavWaypoints);
        // Summed over the waypoints rather than read from _LengthUu, and from the agent outwards on
        // both sides: the two providers disagree about whether the start point is a waypoint, and a
        // length that inherits that disagreement reports a route difference that does not exist.
        Comparison._GroundNavLengthUu = ck_crowd_shadow_compare_processor::Get_PolylineLengthUu(AgentLocation, GroundNavWaypoints);
        Comparison._GroundNavEndpoint = ck_crowd_shadow_compare_processor::Get_Endpoint(GroundNavWaypoints, Goal);
        Comparison._GroundNavSearchMs = GroundNavResult.Get_SearchDurationMs();

        groundnav::shadow::Accumulate(Diagnostics, Comparison, FallbackKey);

        const auto EndpointDeltaUu = FVector::Dist(Comparison._RecastEndpoint, Comparison._GroundNavEndpoint);

        // One line per comparison, so a sweep's divergence report can be reassembled from any run's log
        // whatever order its tests ran in: the diagnostics fragment lives on the PIE world and dies with
        // it, and nothing guarantees the report is read before that happens. Fixed field order, fixed
        // precision per unit and enum names rather than ordinals, for the same reason the report is.
        ck::crowd::Display(
            TEXT("[SHADOW-CMP] {}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}"),
            FixtureKey,
            Comparison._QueryId,
            Comparison._RecastStatus,
            Comparison._RecastFailReason,
            Comparison._GroundNavStatus,
            Comparison._GroundNavFailReason,
            Comparison._RecastWaypointCount,
            Comparison._GroundNavWaypointCount,
            FString::Printf(TEXT("%.3f"), Comparison._RecastLengthUu),
            FString::Printf(TEXT("%.3f"), Comparison._GroundNavLengthUu),
            FString::Printf(TEXT("%.3f"), EndpointDeltaUu),
            FString::Printf(TEXT("%.4f"), Comparison._RecastQueryMs),
            FString::Printf(TEXT("%.4f"), Comparison._GroundNavSearchMs));

        auto NonConstHandle = InHandle;
        NonConstHandle.AddOrGet<FFragment_CrowdAgent_ShadowCompared>()._LastComparedRevision = Revision;
    }
}

// --------------------------------------------------------------------------------------------------------------------
