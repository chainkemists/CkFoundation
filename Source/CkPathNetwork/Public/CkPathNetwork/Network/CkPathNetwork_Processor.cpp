#include "CkPathNetwork_Processor.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/CkPathNetwork_Stats.h"
#include "CkPathNetwork/Network/CkPathNetwork_Build.h"
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
    constexpr auto SideOffsetClampMarginCm = 30.0f;

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

    // Right-hand walking: the side-keeping offset points to the right of the TRAVEL direction, so
    // opposing agents naturally occupy opposite sides of the centerline.
    auto
    Compile_OnRibbonSpan(
        const FBuiltNetwork& InNetwork,
        const FRouteLegSpan& InSpan,
        float InSideKeepingFraction,
        float InWaypointSpacing,
        TArray<FVector>& InOutWaypoints) -> void
    {
        const auto Span = InSpan._ToDist - InSpan._FromDist;
        const auto SpanLength = FMath::Abs(Span);

        if (SpanLength < UE_KINDA_SMALL_NUMBER)
        { return; }

        const auto Direction = Span > 0.0f ? 1.0f : -1.0f;
        const auto StepCount = FMath::Max(1, FMath::CeilToInt32(SpanLength / InWaypointSpacing));
        const auto Step = SpanLength / StepCount;

        for (auto Index = 0; Index <= StepCount; ++Index)
        {
            const auto Dist = InSpan._FromDist + Direction * FMath::Min(Index * Step, SpanLength);
            const auto Sample = InNetwork.Sample_Edge(InSpan._EdgeId, Dist);

            const auto TravelTangent = Sample._Tangent * Direction;
            const auto Right = FVector::CrossProduct(FVector::UpVector, TravelTangent).GetSafeNormal();

            const auto OffsetMagnitude = FMath::Min(
                InSideKeepingFraction * Sample._HalfWidth,
                FMath::Max(0.0f, Sample._HalfWidth - SideOffsetClampMarginCm));

            Append_CompiledWaypoint(InOutWaypoints, Sample._Location + Right * OffsetMagnitude);
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
            const FFragment_PathNetworkFollower_Params& InParams,
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
                if (_BudgetRemainingThisTick <= 0)
                {
                    // Re-queue verbatim: the fragment re-add marks the entity dirty, resuming the drain next tick.
                    // Not yet handled, so the completion delegate must not fire here — it rides the
                    // re-queued copy and fires when a later tick actually drains it.
                    auto NonConstHandle = InHandle;
                    NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(InRequest);
                    return;
                }

                --_BudgetRemainingThisTick;

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

        const auto GoalLocation = InRequest.Get_GoalLocation();

        const auto FailRoute = [&](ECk_PathNetwork_RouteFailReason InReason)
        {
            InCorridor._Result = FCk_PathNetwork_RouteResult{};
            InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Failed;
            InCorridor._Result._FailReason = InReason;
            InCorridor._Result._GoalLocation = GoalLocation;

            auto Follower = InHandle;
            UUtils_Signal_PathNetworkFollower_OnRouteFailed::Broadcast(Follower, MakePayload(Follower));

            ck::pathnetwork::Warning(TEXT("PathNetworkFollower [{}] route to {} failed: [{}]"),
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

        for (auto SpanIndex = 0; SpanIndex < AcceptedSpans.Num(); ++SpanIndex)
        {
            const auto& Span = AcceptedSpans[SpanIndex];

            auto Leg = FCk_PathNetwork_CorridorLeg{};

            if (Span._IsOffPath)
            {
                Leg._LegType = ECk_PathNetwork_CorridorLegType::OffPath;
                Leg._Waypoints = AcceptedOffPathWaypoints.FindChecked(SpanIndex);

                for (const auto& Waypoint : Leg.Get_Waypoints())
                { Append_CompiledWaypoint(Result._CompiledWaypoints, Waypoint); }
            }
            else
            {
                Leg._LegType = ECk_PathNetwork_CorridorLegType::OnRibbon;
                Leg._EdgeId = Span._EdgeId;
                Leg._EntryDistAlong = Span._FromDist;
                Leg._ExitDistAlong = Span._ToDist;

                Compile_OnRibbonSpan(
                    BuiltNetwork,
                    Span,
                    InParams.Get_SideKeepingFraction(),
                    InParams.Get_CorridorWaypointSpacing(),
                    Result._CompiledWaypoints);
            }

            Result._Legs.Add(MoveTemp(Leg));
        }

        const auto FirstWaypointIsUnderTheAgent = Result._CompiledWaypoints.Num() > 1 &&
            FVector::Dist(Result._CompiledWaypoints[0], StartLocation) < InParams.Get_CorridorWaypointSpacing() * 0.5f;

        if (FirstWaypointIsUnderTheAgent)
        { Result._CompiledWaypoints.RemoveAt(0); }

        // Leg seams emit near-coincident waypoints (an off-path leg ends at the centerline projection,
        // the next ribbon sample sits at the same arc-length but laterally offset). An agent whose turn
        // radius exceeds its arrival radius orbits such a lateral micro-hop forever.
        if (Result._CompiledWaypoints.Num() > 2)
        {
            const auto MinSeparation = InParams.Get_CorridorWaypointSpacing() * 0.75f;

            auto Filtered = TArray<FVector>{};
            Filtered.Reserve(Result._CompiledWaypoints.Num());
            Filtered.Add(Result._CompiledWaypoints[0]);

            for (auto WaypointIndex = 1; WaypointIndex < Result._CompiledWaypoints.Num() - 1; ++WaypointIndex)
            {
                if (FVector::Dist(Result._CompiledWaypoints[WaypointIndex], Filtered.Last()) >= MinSeparation)
                { Filtered.Add(Result._CompiledWaypoints[WaypointIndex]); }
            }

            const auto& FinalWaypoint = Result._CompiledWaypoints.Last();

            if (Filtered.Num() > 1 && FVector::Dist(FinalWaypoint, Filtered.Last()) < MinSeparation)
            { Filtered.Last() = FinalWaypoint; }
            else
            { Filtered.Add(FinalWaypoint); }

            Result._CompiledWaypoints = MoveTemp(Filtered);
        }

        if (Result._CompiledWaypoints.IsEmpty())
        { Result._CompiledWaypoints.Add(GoalLocation); }
        else
        { Result._CompiledWaypoints.Last() = GoalLocation; }

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

        auto NonConstHandle = InHandle;
        NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(Request);

        InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Pending;
        InCorridor._NetworkEpoch = CurrentEpoch;

        ck::pathnetwork::Verbose(TEXT("PathNetworkFollower [{}] corridor invalidated by network rebuild (epoch [{}]) — replanning"),
            InHandle, CurrentEpoch);
    }
}

// --------------------------------------------------------------------------------------------------------------------
