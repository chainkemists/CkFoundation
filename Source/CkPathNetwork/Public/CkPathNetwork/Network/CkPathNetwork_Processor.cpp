#include "CkPathNetwork_Processor.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/CkPathNetwork_Stats.h"
#include "CkPathNetwork/Network/CkPathNetwork_Build.h"
#include "CkPathNetwork/Network/CkPathNetwork_CorridorCompile.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"
#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"
#include "CkPathNetwork/Settings/CkPathNetwork_ProjectSettings.h"

#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/Nav/CkNav_Algorithm.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>

#include <array>
#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetwork_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetwork_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetwork_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetworkFollower_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetworkFollower_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetworkFollower_InvalidateOnRebuild);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("PathNetwork::Setup"),            STAT_CkPathNetwork_Setup,            STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::HandleRequests"),   STAT_CkPathNetwork_HandleRequests,   STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::PlanRoute"),        STAT_CkPathNetwork_PlanRoute,        STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::Invalidate"),       STAT_CkPathNetwork_Invalidate,       STATGROUP_CkPathNetwork);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_processor
{
    using namespace ck::pathnetwork;

    constexpr auto SearchIterationCap = 200000;
    constexpr auto CompiledWaypointMergeDistance = 1.0f;
    constexpr auto ClearanceProjectionPlanarExtentCm = 25.0f;
    constexpr auto ClearanceProjectionVerticalExtentCm = 100.0f;
    constexpr auto ClearanceImprovementEpsilonCm = 1.0f;

    // Outcome drives repricing: PathFailed prices the hop out, NoNavmesh legs are never repriced
    // (they fall back to a straight line).
    enum class EOffPathResolve : uint8
    {
        NoNavmesh,
        PathFailed,
        Resolved
    };

    struct FOffPathLegResolution
    {
        TArray<FVector> _Waypoints;
        float _Length = 0.0f;
        EOffPathResolve _Outcome = EOffPathResolve::NoNavmesh;
    };

    // Direct FindPathSync (bypassing the CkNavigation request path) is safe here: every caller drains
    // under the route processor's per-frame budget (_MaxRouteQueriesPerFrame).
    auto
    Resolve_OffPathLeg(UWorld* InWorld, const FVector& InFrom, const FVector& InTo) -> FOffPathLegResolution
    {
        auto Result = FOffPathLegResolution{};
        Result._Waypoints = {InFrom, InTo};
        Result._Length = static_cast<float>(FVector::Dist(InFrom, InTo));

        auto* NavSys = IsValid(InWorld) ? UNavigationSystemV1::GetCurrent(InWorld) : nullptr;
        auto* NavData = (NavSys != nullptr)
            ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;

        if (NavSys == nullptr || NavData == nullptr)
        { return Result; }

        auto NavResult = FCk_Nav_PathResult{};

        constexpr auto AllowPartial = false;
        constexpr auto AgentRadiusForFirstSkip = 0.0f;

        const auto FoundPath = FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            InFrom,
            InTo,
            AllowPartial,
            UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent(),
            UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent(),
            AgentRadiusForFirstSkip,
            NavResult);

        if (NOT FoundPath)
        {
            Result._Outcome = EOffPathResolve::PathFailed;
            return Result;
        }

        const auto& Waypoints = NavResult.Get_Waypoints();

        const auto StartIsEffectivelyTheEnd = Waypoints.Num() < 2;

        if (StartIsEffectivelyTheEnd)
        {
            Result._Outcome = EOffPathResolve::Resolved;
            return Result;
        }

        auto Length = 0.0f;
        for (auto Index = 0; Index < Waypoints.Num() - 1; ++Index)
        { Length += static_cast<float>(FVector::Dist(Waypoints[Index], Waypoints[Index + 1])); }

        Result._Waypoints = Waypoints;
        Result._Length = Length;
        Result._Outcome = EOffPathResolve::Resolved;
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Gather_Candidates(const FBuiltNetwork& InNetwork, const FVector& InLocation) -> TArray<FRouteOverlayPoint>
    {
        const auto CandidateCount = UCk_Utils_PathNetwork_Settings_UE::Get_GoalCandidateCount();
        const auto MaxDoublings = UCk_Utils_PathNetwork_Settings_UE::Get_CandidateSearchMaxDoublings();

        auto Radius = UCk_Utils_PathNetwork_Settings_UE::Get_CandidateSearchRadius();
        auto EdgeIds = TArray<int32>{};

        for (auto Doubling = 0; Doubling <= MaxDoublings; ++Doubling)
        {
            EdgeIds = InNetwork.Query_EdgesNear(InLocation, Radius);

            if (EdgeIds.Num() >= CandidateCount)
            { break; }

            Radius *= 2.0f;
        }

        struct FScoredCandidate
        {
            FRouteOverlayPoint _Point;
            float _Distance = 0.0f;
        };

        auto Scored = TArray<FScoredCandidate>{};
        Scored.Reserve(EdgeIds.Num());

        for (const auto EdgeId : EdgeIds)
        {
            const auto Projection = InNetwork.Project_OntoEdge(EdgeId, InLocation);
            Scored.Add(FScoredCandidate{
                FRouteOverlayPoint{EdgeId, Projection._DistAlong, Projection._Location},
                Projection._Distance});
        }

        Scored.Sort([](const FScoredCandidate& InA, const FScoredCandidate& InB)
        { return InA._Distance < InB._Distance; });

        auto Result = TArray<FRouteOverlayPoint>{};
        for (auto Index = 0; Index < Scored.Num() && Index < CandidateCount; ++Index)
        { Result.Add(Scored[Index]._Point); }

        return Result;
    }

    auto
    Merge_CandidatesIntoOverlay(FRouteGraphSharedData& InOutShared, const TArray<FRouteOverlayPoint>& InCandidates) -> void
    {
        constexpr auto DedupeDistAlong = 1.0f;

        for (const auto& Candidate : InCandidates)
        {
            auto IsDuplicate = false;

            if (const auto* ExistingOnEdge = InOutShared._OverlayPointsByEdge.Find(Candidate._EdgeId))
            {
                for (const auto ExistingIndex : *ExistingOnEdge)
                {
                    if (FMath::Abs(InOutShared._OverlayPoints[ExistingIndex]._DistAlong - Candidate._DistAlong) < DedupeDistAlong)
                    {
                        IsDuplicate = true;
                        break;
                    }
                }
            }

            if (IsDuplicate)
            { continue; }

            const auto NewIndex = InOutShared._OverlayPoints.Add(Candidate);
            InOutShared._OverlayPointsByEdge.FindOrAdd(Candidate._EdgeId).Add(NewIndex);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Append_CompiledWaypoint(TArray<FVector>& InOutWaypoints, const FVector& InPoint) -> void
    {
        if (InOutWaypoints.Num() > 0 &&
            FVector::Dist(InOutWaypoints.Last(), InPoint) < CompiledWaypointMergeDistance)
        { return; }

        InOutWaypoints.Add(InPoint);
    }

    auto
    Get_DefaultRecastNavmesh(UWorld* InWorld) -> ARecastNavMesh*
    {
        auto* NavSys = IsValid(InWorld) ? UNavigationSystemV1::GetCurrent(InWorld) : nullptr;
        return NavSys != nullptr
            ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;
    }

    auto
    Is_NavmeshSegmentValid(
        const ARecastNavMesh& InNavData,
        const FSharedConstNavQueryFilter& InQueryFilter,
        const FVector& InFrom,
        const FVector& InTo) -> bool
    {
        auto HitLocation = FVector::ZeroVector;
        auto RaycastResult = ARecastNavMesh::FRaycastResult{};
        const auto HitNavmeshBoundary = ARecastNavMesh::NavMeshRaycast(
            &InNavData,
            InFrom,
            InTo,
            HitLocation,
            InQueryFilter,
            nullptr,
            RaycastResult);

        // Every endpoint is projected before this call, so a boundary hit is the
        // discriminating evidence that the segment leaves the walkable corridor.
        // Do not require bIsRaycastEndInCorridor: Recast can select a different
        // containing polygon for an endpoint on a shared seam and report a false
        // mismatch even though the ray reached it without crossing a boundary.
        return NOT HitNavmeshBoundary;
    }

    auto
    Try_ProjectPathOntoNavmesh(UWorld* InWorld, TArray<FVector>& InOutWaypoints) -> bool
    {
        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return true; }

        if (NOT NavData->GetDefaultQueryFilter().IsValid())
        { return false; }

        const auto QueryFilter = NavData->GetDefaultQueryFilter();
        const auto ProjectionExtent = FVector{
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionVerticalExtentCm};

        for (auto WaypointIndex = 0; WaypointIndex < InOutWaypoints.Num(); ++WaypointIndex)
        {
            const auto AuthoredWaypoint = InOutWaypoints[WaypointIndex];
            auto ProjectedWaypoint = FNavLocation{};
            if (NOT NavData->ProjectPoint(
                    AuthoredWaypoint,
                    ProjectedWaypoint,
                    ProjectionExtent,
                    QueryFilter))
            {
                ck::pathnetwork::Verbose(
                    TEXT("PathNetwork nav normalization failed to project waypoint [{}] at [{}]"),
                    WaypointIndex,
                    AuthoredWaypoint);
                return false;
            }

            if (FVector::Dist2D(ProjectedWaypoint.Location, AuthoredWaypoint) >
                    ClearanceProjectionPlanarExtentCm ||
                FMath::Abs(ProjectedWaypoint.Location.Z - AuthoredWaypoint.Z) >
                    ClearanceProjectionVerticalExtentCm)
            {
                ck::pathnetwork::Verbose(
                    TEXT("PathNetwork nav normalization rejected waypoint [{}]: [{}] -> [{}]"),
                    WaypointIndex,
                    AuthoredWaypoint,
                    ProjectedWaypoint.Location);
                return false;
            }

            // Publish the projected point itself. Validating a projected copy while leaving the
            // authored point in the movement path would recreate the airborne-route defect.
            InOutWaypoints[WaypointIndex] = ProjectedWaypoint.Location;
        }

        return true;
    }

    auto
    Is_NavmeshPathValid(UWorld* InWorld, TConstArrayView<FVector> InWaypoints) -> bool
    {
        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return true; }

        if (NOT NavData->GetDefaultQueryFilter().IsValid())
        { return false; }

        const auto QueryFilter = NavData->GetDefaultQueryFilter();
        for (auto Index = 0; Index < InWaypoints.Num() - 1; ++Index)
        {
            if (NOT Is_NavmeshSegmentValid(
                *NavData,
                QueryFilter,
                InWaypoints[Index],
                InWaypoints[Index + 1]))
            {
                ck::pathnetwork::Verbose(
                    TEXT("PathNetwork nav validation rejected segment [{}] from [{}] to [{}]"),
                    Index,
                    InWaypoints[Index],
                    InWaypoints[Index + 1]);
                return false;
            }
        }

        return true;
    }

    auto
    Apply_NavmeshClearance(
        UWorld* InWorld,
        const FBuiltNetwork& InNetwork,
        TConstArrayView<FRouteLegSpan> InSpans,
        float InDesiredClearance,
        TArray<FVector>& InOutWaypoints) -> void
    {
        if (InDesiredClearance <= UE_KINDA_SMALL_NUMBER || InOutWaypoints.Num() < 3)
        { return; }

        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr ||
            NOT NavData->HasValidNavmesh() ||
            NOT NavData->GetDefaultQueryFilter().IsValid())
        { return; }

        const auto QueryFilter = NavData->GetDefaultQueryFilter();
        const auto ProjectionExtent = FVector{
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionVerticalExtentCm};
        const auto SentinelValue = TNumericLimits<FVector::FReal>::Max();
        const auto Sentinel = FVector{SentinelValue, SentinelValue, SentinelValue};

        for (auto WaypointIndex = 1; WaypointIndex < InOutWaypoints.Num() - 1; ++WaypointIndex)
        {
            const auto Original = InOutWaypoints[WaypointIndex];
            auto ClosestWall = Sentinel;
            const auto CurrentClearance = static_cast<float>(NavData->FindDistanceToWall(
                Original,
                QueryFilter,
                InDesiredClearance,
                &ClosestWall));

            const auto FoundNearbyWall = ClosestWall.X != SentinelValue;
            if (NOT FoundNearbyWall || CurrentClearance >= InDesiredClearance - ClearanceImprovementEpsilonCm)
            { continue; }

            auto AwayFromWall = Original - ClosestWall;
            AwayFromWall.Z = 0.0;
            AwayFromWall = AwayFromWall.GetSafeNormal();
            if (AwayFromWall.IsNearlyZero())
            { continue; }

            const auto RequestedShift = InDesiredClearance - CurrentClearance;
            constexpr auto AttemptFractions = std::array{1.0f, 0.5f, 0.25f};

            for (const auto Fraction : AttemptFractions)
            {
                const auto UnprojectedCandidate =
                    Original + AwayFromWall * RequestedShift * Fraction;
                auto ProjectedCandidate = FNavLocation{};
                if (NOT NavData->ProjectPoint(
                    UnprojectedCandidate,
                    ProjectedCandidate,
                    ProjectionExtent,
                    QueryFilter))
                { continue; }

                auto Candidate = ProjectedCandidate.Location;
                if (FVector::Dist2D(Candidate, UnprojectedCandidate) >
                        ClearanceProjectionPlanarExtentCm ||
                    FMath::Abs(Candidate.Z - UnprojectedCandidate.Z) >
                        ClearanceProjectionVerticalExtentCm)
                { continue; }

                if (NOT Is_PointInsideRibbonRun(InNetwork, InSpans, Candidate) ||
                    NOT Is_SegmentInsideRibbonRun(
                        InNetwork,
                        InSpans,
                        InOutWaypoints[WaypointIndex - 1],
                        Candidate) ||
                    NOT Is_SegmentInsideRibbonRun(
                        InNetwork,
                        InSpans,
                        Candidate,
                        InOutWaypoints[WaypointIndex + 1]))
                { continue; }

                if (NOT Is_NavmeshSegmentValid(
                        *NavData,
                        QueryFilter,
                        InOutWaypoints[WaypointIndex - 1],
                        Candidate) ||
                    NOT Is_NavmeshSegmentValid(
                        *NavData,
                        QueryFilter,
                        Candidate,
                        InOutWaypoints[WaypointIndex + 1]))
                { continue; }

                auto CandidateClosestWall = Sentinel;
                const auto CandidateClearance = static_cast<float>(NavData->FindDistanceToWall(
                    Candidate,
                    QueryFilter,
                    InDesiredClearance,
                    &CandidateClosestWall));
                const auto CandidateHasNearbyWall = CandidateClosestWall.X != SentinelValue;
                if (CandidateHasNearbyWall &&
                    CandidateClearance <= CurrentClearance + ClearanceImprovementEpsilonCm)
                { continue; }

                InOutWaypoints[WaypointIndex] = Candidate;
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_PathNetwork_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_Setup);

        InGraph._Network = pathnetwork::Build_NetworkFromRibbons(InParams.Get_Ribbons(), InParams.Get_BuildParams());
        InGraph._Epoch += 1;

        auto NonConstHandle = InHandle;
        NonConstHandle.Try_Remove<FTag_PathNetwork_NeedsBuild>();

        ck::pathnetwork::Display(TEXT("PathNetwork [{}] built: [{}] ribbons -> [{}] nodes, [{}] edges (epoch [{}])"),
            InHandle, InParams.Get_Ribbons().Num(), InGraph.Get_Network()._Nodes.Num(),
            InGraph.Get_Network()._Edges.Num(), InGraph.Get_Epoch());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetwork_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            FFragment_PathNetwork_Requests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_HandleRequests);

        InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
        {
            algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
            [&](const auto& InRequest)
            {
                // DoHandleRequest is void and has no rejection path, so reaching the line after the
                // call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InGraph, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;
            }), policy::DontResetContainer{});
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetwork_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            const FCk_Request_PathNetwork_Rebuild& InRequest)
        -> void
    {
        InParams._Ribbons = InRequest.Get_NewRibbons();
        InGraph._Network = pathnetwork::Build_NetworkFromRibbons(InParams.Get_Ribbons(), InParams.Get_BuildParams());
        InGraph._Epoch += 1;

        ck::pathnetwork::Display(TEXT("PathNetwork [{}] rebuilt: [{}] ribbons -> [{}] nodes, [{}] edges (epoch [{}])"),
            InHandle, InParams.Get_Ribbons().Num(), InGraph.Get_Network()._Nodes.Num(),
            InGraph.Get_Network()._Edges.Num(), InGraph.Get_Epoch());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetwork_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetworkFollower_HandleRequests::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        _BudgetRemainingThisTick = UCk_Utils_PathNetwork_Settings_UE::Get_MaxRouteQueriesPerFrame();
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_PathNetworkFollower_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_PathNetworkFollower_Requests& InRequests) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_PlanRoute);

        InHandle.CopyAndRemove(InRequests, [&](const auto& InSnapshot)
        {
            algo::ForEachRequest(InSnapshot._Requests, ck::Visitor(
            [&](const auto& InRequest)
            {
                if constexpr (NOT std::is_same_v<
                    std::decay_t<decltype(InRequest)>,
                    FCk_Request_PathNetworkFollower_UpdateTuning>)
                {
                    if (_BudgetRemainingThisTick <= 0)
                    {
                        // Out of budget: re-queue verbatim; the fragment re-add marks the entity dirty
                        // so the drain resumes next tick. The completion delegate rides the re-queued
                        // copy and fires only when a later tick actually drains it.
                        auto NonConstHandle = InHandle;
                        NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(InRequest);
                        return;
                    }

                    --_BudgetRemainingThisTick;
                }

                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InParams, InCorridor, InRequest, Result);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_PathNetworkFollower_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            const FCk_Request_PathNetworkFollower_FindRoute& InRequest,
            ECk_Request_OperationResult& OutResult) const
        -> void
    {
        using namespace ck_pathnetwork_processor;
        using namespace ck::pathnetwork;

        if (InRequest.Get_TuningRevision() != InParams.Get_TuningRevision())
        { return; }

        const auto GoalLocation = InRequest.Get_GoalLocation();

        const auto FailRoute = [&](ECk_PathNetwork_RouteFailReason InReason)
        {
            InCorridor._Result = FCk_PathNetwork_RouteResult{};
            InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Failed;
            InCorridor._Result._FailReason = InReason;
            InCorridor._Result._GoalLocation = GoalLocation;
            InCorridor._Result._TuningRevision = InRequest.Get_TuningRevision();

            auto Follower = InHandle;
            UUtils_Signal_PathNetworkFollower_OnRouteFailed::Broadcast(Follower, MakePayload(Follower));

            // Route failure is a first-class result delivered by OnRouteFailed, not a framework
            // fault. Keep it visible without escalating expected unreachable-goal tests.
            ck::pathnetwork::Display(TEXT("PathNetworkFollower [{}] route to {} failed: [{}]"),
                InHandle, GoalLocation, InReason);
        };

        auto Network = ck::IsValid(InRequest.Get_Network()) ? InRequest.Get_Network() : InParams.Get_Network();

        if (ck::Is_NOT_Valid(Network) || NOT UCk_Utils_PathNetwork_UE::Has(Network))
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoNetwork);
            return;
        }

        const auto& GraphFragment = Network.Get<FFragment_PathNetwork_Graph>();
        const auto& BuiltNetwork = GraphFragment.Get_Network();

        if (GraphFragment.Get_Epoch() <= 0 || BuiltNetwork._Edges.IsEmpty())
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NetworkNotBuilt);
            return;
        }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
            TEXT("PathNetworkFollower [{}] has no Transform — cannot resolve a route start location"), InHandle)
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

        auto Shared = MakeShared<FRouteGraphSharedData>();
        Merge_CandidatesIntoOverlay(*Shared, Gather_Candidates(BuiltNetwork, StartLocation));
        Merge_CandidatesIntoOverlay(*Shared, Gather_Candidates(BuiltNetwork, GoalLocation));

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto StartId = FRouteNodeId{ERouteNodeKind::Start, 0};
        const auto GoalId = FRouteNodeId{ERouteNodeKind::Goal, 0};

        const auto MaxRepriceIterations = UCk_Utils_PathNetwork_Settings_UE::Get_MaxRepriceIterations();
        const auto RepriceTolerance = UCk_Utils_PathNetwork_Settings_UE::Get_RepriceTolerance();
        const auto Multiplier = InParams.Get_OffPathCostMultiplier();

        auto AcceptedSpans = TArray<FRouteLegSpan>{};
        auto AcceptedOffPathWaypoints = TMap<int32, TArray<FVector>>{};
        auto AcceptedCost = 0.0f;
        auto RouteFound = false;

        for (auto Iteration = 0; Iteration <= MaxRepriceIterations; ++Iteration)
        {
            const auto Graph = FRouteGraph{&BuiltNetwork, StartLocation, GoalLocation, Multiplier, Shared};

            auto Search = astar::TSearchState<FRouteNodeId, FRouteGraph>{Graph, StartId, GoalId};

            auto SearchParams = astar::FSearchParams{};
            SearchParams.MaxIterationsPerTick = SearchIterationCap;

            if (Search.ContinueSearch(SearchParams) != astar::ESearchStatus::Complete)
            { break; }

            const auto Spans = ExtractLegSpans(Graph, BuiltNetwork, Search.GetResultPath());

            auto OffPathWaypoints = TMap<int32, TArray<FVector>>{};
            auto NeedsReprice = false;

            for (auto SpanIndex = 0; SpanIndex < Spans.Num(); ++SpanIndex)
            {
                const auto& Span = Spans[SpanIndex];

                if (NOT Span._IsOffPath)
                { continue; }

                const auto Resolution = Resolve_OffPathLeg(World, Span._FromLocation, Span._ToLocation);
                OffPathWaypoints.Add(SpanIndex, Resolution._Waypoints);

                if (Iteration >= MaxRepriceIterations)
                { continue; }

                const auto Key = FRouteGraph::PackOffPathKey(Span._FromId, Span._ToId);
                const auto Euclidean = static_cast<float>(FVector::Dist(Span._FromLocation, Span._ToLocation));

                switch (Resolution._Outcome)
                {
                    case EOffPathResolve::PathFailed:
                    {
                        const auto PriceOutBlockedHop = TNumericLimits<float>::Max() / 8.0f;
                        Shared->_RepricedOffPathCosts.Add(Key, PriceOutBlockedHop);
                        NeedsReprice = true;
                        break;
                    }
                    case EOffPathResolve::Resolved:
                    {
                        if (Resolution._Length > Euclidean * RepriceTolerance)
                        {
                            Shared->_RepricedOffPathCosts.Add(Key, Resolution._Length * Multiplier);
                            NeedsReprice = true;
                        }
                        break;
                    }
                    case EOffPathResolve::NoNavmesh:
                    default:
                        break;
                }
            }

            if (NOT NeedsReprice || Iteration >= MaxRepriceIterations)
            {
                AcceptedSpans = Spans;
                AcceptedOffPathWaypoints = MoveTemp(OffPathWaypoints);
                AcceptedCost = Search.GetResultCost();
                RouteFound = true;
                break;
            }
        }

        if (NOT RouteFound)
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        auto Result = FCk_PathNetwork_RouteResult{};
        Result._Status = ECk_PathNetwork_RouteStatus::Ready;
        Result._TotalCost = AcceptedCost;
        Result._GoalLocation = GoalLocation;
        Result._TuningRevision = InRequest.Get_TuningRevision();

        for (auto SpanIndex = 0; SpanIndex < AcceptedSpans.Num();)
        {
            const auto& Span = AcceptedSpans[SpanIndex];

            if (Span._IsOffPath)
            {
                auto Leg = FCk_PathNetwork_CorridorLeg{};
                Leg._LegType = ECk_PathNetwork_CorridorLegType::OffPath;
                Leg._Waypoints = AcceptedOffPathWaypoints.FindChecked(SpanIndex);

                for (const auto& Waypoint : Leg.Get_Waypoints())
                { Append_CompiledWaypoint(Result._CompiledWaypoints, Waypoint); }
                Result._Legs.Add(MoveTemp(Leg));
                ++SpanIndex;
            }
            else
            {
                const auto RunStartIndex = SpanIndex;
                while (SpanIndex < AcceptedSpans.Num() && NOT AcceptedSpans[SpanIndex]._IsOffPath)
                {
                    const auto& RunSpan = AcceptedSpans[SpanIndex];
                    auto Leg = FCk_PathNetwork_CorridorLeg{};
                    Leg._LegType = ECk_PathNetwork_CorridorLegType::OnRibbon;
                    Leg._EdgeId = RunSpan._EdgeId;
                    Leg._EntryDistAlong = RunSpan._FromDist;
                    Leg._ExitDistAlong = RunSpan._ToDist;
                    Result._Legs.Add(MoveTemp(Leg));
                    ++SpanIndex;
                }

                const auto RunSpans = TConstArrayView<FRouteLegSpan>{
                    AcceptedSpans.GetData() + RunStartIndex,
                    SpanIndex - RunStartIndex};
                auto CompileParams = FCorridorCompileParams{};
                CompileParams._SideKeepingFraction = InParams.Get_SideKeepingFraction();
                CompileParams._WaypointSpacing = InParams.Get_CorridorWaypointSpacing();
                CompileParams._CornerSmoothingDistance = InParams.Get_CornerSmoothingDistance();
                CompileParams._RampSideOffsetAtStart =
                    RunStartIndex > 0 && AcceptedSpans[RunStartIndex - 1]._IsOffPath;
                CompileParams._RampSideOffsetAtEnd =
                    SpanIndex < AcceptedSpans.Num() && AcceptedSpans[SpanIndex]._IsOffPath;

                auto RunWaypoints = TArray<FVector>{};
                constexpr auto CompileAttempts = 3;
                for (auto CompileAttempt = 0; CompileAttempt < CompileAttempts; ++CompileAttempt)
                {
                    auto AttemptParams = CompileParams;
                    if (CompileAttempt >= 1)
                    { AttemptParams._CornerSmoothingDistance = 0.0f; }
                    if (CompileAttempt >= 2)
                    { AttemptParams._SideKeepingFraction = 0.0f; }

                    auto CandidateWaypoints = Compile_OnRibbonRun(
                        BuiltNetwork,
                        RunSpans,
                        AttemptParams);
                    if (CandidateWaypoints.Num() < 2)
                    { continue; }

                    if (NOT Try_ProjectPathOntoNavmesh(World, CandidateWaypoints))
                    { continue; }

                    auto ProjectedPathContained = true;
                    for (auto WaypointIndex = 0;
                         WaypointIndex < CandidateWaypoints.Num() - 1;
                         ++WaypointIndex)
                    {
                        if (NOT Is_SegmentInsideRibbonRun(
                                BuiltNetwork,
                                RunSpans,
                                CandidateWaypoints[WaypointIndex],
                                CandidateWaypoints[WaypointIndex + 1]))
                        {
                            ProjectedPathContained = false;
                            break;
                        }
                    }
                    if (NOT ProjectedPathContained)
                    { continue; }

                    Apply_NavmeshClearance(
                        World,
                        BuiltNetwork,
                        RunSpans,
                        InParams.Get_DesiredNavmeshClearance(),
                        CandidateWaypoints);
                    if (Is_NavmeshPathValid(World, CandidateWaypoints))
                    {
                        RunWaypoints = MoveTemp(CandidateWaypoints);
                        break;
                    }
                }

                if (RunWaypoints.Num() < 2)
                {
                    FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                    return;
                }

                for (const auto& Waypoint : RunWaypoints)
                { Append_CompiledWaypoint(Result._CompiledWaypoints, Waypoint); }
            }
        }

        if (NOT Try_ProjectPathOntoNavmesh(World, Result._CompiledWaypoints))
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        auto NormalizedStart = TArray<FVector>{StartLocation};
        if (NOT Try_ProjectPathOntoNavmesh(World, NormalizedStart))
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        // The agent stands at the first waypoint already — drop it so movement consumers don't
        // backtrack (same artifact CkNavigation's first-waypoint skip addresses).
        if (Result._CompiledWaypoints.Num() > 1 &&
            FVector::Dist(Result._CompiledWaypoints[0], NormalizedStart[0]) <
                CompiledWaypointMergeDistance)
        { Result._CompiledWaypoints.RemoveAt(0); }

        auto NormalizedGoal = TArray<FVector>{GoalLocation};
        if (NOT Try_ProjectPathOntoNavmesh(World, NormalizedGoal))
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        const auto HasExactTerminal =
            Result._CompiledWaypoints.Num() > 0 &&
            FVector::Dist(Result._CompiledWaypoints.Last(), NormalizedGoal[0]) <
                CompiledWaypointMergeDistance;
        if (NOT HasExactTerminal)
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }
        Result._CompiledWaypoints.Last() = NormalizedGoal[0];

        auto FullMovementPath = Result._CompiledWaypoints;
        FullMovementPath.Insert(NormalizedStart[0], 0);
        if (NOT Is_NavmeshPathValid(World, FullMovementPath))
        {
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        InCorridor._Result = MoveTemp(Result);
        InCorridor._Network = Network;
        InCorridor._NetworkEpoch = GraphFragment.Get_Epoch();

        auto Follower = InHandle;
        UUtils_Signal_PathNetworkFollower_OnRouteReady::Broadcast(Follower, MakePayload(Follower, InCorridor.Get_Result()));

        ck::pathnetwork::Verbose(TEXT("PathNetworkFollower [{}] route to {} ready: [{}] legs, [{}] waypoints, cost [{}]"),
            InHandle, GoalLocation, InCorridor.Get_Result().Get_Legs().Num(),
            InCorridor.Get_Result().Get_CompiledWaypoints().Num(), InCorridor.Get_Result().Get_TotalCost());

        OutResult = ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetworkFollower_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetworkFollower_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            const FCk_Request_PathNetworkFollower_UpdateTuning& InRequest,
            ECk_Request_OperationResult& OutResult) const
        -> void
    {
        const auto& Tuning = InRequest.Get_Tuning();
        // The public entrypoint validated this payload before enqueueing. Keep the
        // processor total against malformed/stale requests from native callers.
        const auto Multiplier = Tuning.Get_OffPathCostMultiplier();
        const auto SideKeeping = Tuning.Get_SideKeepingFraction();
        const auto Spacing = Tuning.Get_CorridorWaypointSpacing();
        const auto Smoothing = Tuning.Get_CornerSmoothingDistance();
        const auto Clearance = Tuning.Get_DesiredNavmeshClearance();
        const bool TuningIsValid = FMath::IsFinite(Multiplier) && Multiplier >= 1.0f
            && FMath::IsFinite(SideKeeping) && SideKeeping >= 0.0f && SideKeeping <= 0.9f
            && FMath::IsFinite(Spacing) && Spacing >= 50.0f
            && FMath::IsFinite(Smoothing) && Smoothing >= 0.0f && Smoothing <= 1000.0f
            && FMath::IsFinite(Clearance) && Clearance >= 0.0f && Clearance <= 1000.0f;
        CK_ENSURE_IF_NOT(TuningIsValid,
            TEXT("PathNetworkFollower tuning request contains invalid values "
                 "(multiplier [{}], side [{}], spacing [{}], smoothing [{}], clearance [{}])"),
            Multiplier, SideKeeping, Spacing, Smoothing, Clearance)
        {}
        if (NOT TuningIsValid)
        { return; }

        InParams.Set_OffPathCostMultiplier(Multiplier);
        InParams.Set_SideKeepingFraction(SideKeeping);
        InParams.Set_CorridorWaypointSpacing(Spacing);
        InParams.Set_CornerSmoothingDistance(Smoothing);
        InParams.Set_DesiredNavmeshClearance(Clearance);
        ++InParams._TuningRevision;
        OutResult = ECk_Request_OperationResult::Succeeded;

        const auto Status = InCorridor._Result.Get_Status();
        if (Status != ECk_PathNetwork_RouteStatus::Ready && Status != ECk_PathNetwork_RouteStatus::Pending)
        { return; }

        auto Replan = FCk_Request_PathNetworkFollower_FindRoute{InCorridor._Result.Get_GoalLocation()};
        Replan.Set_Network(InCorridor._Network);
        Replan.Set_TuningRevision(InParams.Get_TuningRevision());
        auto NonConstHandle = InHandle;
        NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(Replan);
        InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Pending;
        InCorridor._Result._TuningRevision = InParams.Get_TuningRevision();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetworkFollower_InvalidateOnRebuild::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor) const
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_Invalidate);

        if (InCorridor.Get_Result().Get_Status() != ECk_PathNetwork_RouteStatus::Ready)
        { return; }

        const auto& Network = InCorridor.Get_Network();

        if (ck::Is_NOT_Valid(Network) || NOT Network.Has<FFragment_PathNetwork_Graph>())
        { return; }

        const auto CurrentEpoch = Network.Get<FFragment_PathNetwork_Graph>().Get_Epoch();

        if (CurrentEpoch == InCorridor.Get_NetworkEpoch())
        { return; }

        auto Request = FCk_Request_PathNetworkFollower_FindRoute{InCorridor.Get_Result().Get_GoalLocation()};
        Request.Set_Network(InCorridor.Get_Network());
        Request.Set_TuningRevision(InParams.Get_TuningRevision());

        auto NonConstHandle = InHandle;
        NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(Request);

        InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Pending;
        InCorridor._NetworkEpoch = CurrentEpoch;

        ck::pathnetwork::Verbose(TEXT("PathNetworkFollower [{}] corridor invalidated by network rebuild (epoch [{}]) — replanning"),
            InHandle, CurrentEpoch);
    }
}

// --------------------------------------------------------------------------------------------------------------------
