#include "CkPathNetwork_Processor.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/CkPathNetwork_Stats.h"
#include "CkPathNetwork/Network/CkPathNetwork_Build.h"
#include "CkPathNetwork/Network/CkPathNetwork_CorridorCompile.h"
#include "CkPathNetwork/Network/CkPathNetwork_RouteGraph.h"
#include "CkPathNetwork/Network/CkPathNetwork_RoutePlan.h"
#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"
#include "CkPathNetwork/Settings/CkPathNetwork_ProjectSettings.h"

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

    enum class ERouteFailureStage : uint8
    {
        NotResolved,
        NoNetwork,
        NetworkNotBuilt,
        MissingTransform,
        GraphSearch,
        OffPathNavValidation,
        RibbonCompileEmpty,
        RibbonProjection,
        RibbonContainment,
        RibbonNavValidation,
        CompiledPathProjection,
        StartProjection,
        GoalProjection,
        TerminalMismatch,
        FullPathValidation
    };

    auto
    Get_RouteFailureStageName(
        ERouteFailureStage InStage)
        -> const TCHAR*
    {
        switch (InStage)
        {
            case ERouteFailureStage::NoNetwork: return TEXT("NoNetwork");
            case ERouteFailureStage::NetworkNotBuilt: return TEXT("NetworkNotBuilt");
            case ERouteFailureStage::MissingTransform: return TEXT("MissingTransform");
            case ERouteFailureStage::GraphSearch: return TEXT("GraphSearch");
            case ERouteFailureStage::OffPathNavValidation: return TEXT("OffPathNavValidation");
            case ERouteFailureStage::RibbonCompileEmpty: return TEXT("RibbonCompileEmpty");
            case ERouteFailureStage::RibbonProjection: return TEXT("RibbonProjection");
            case ERouteFailureStage::RibbonContainment: return TEXT("RibbonContainment");
            case ERouteFailureStage::RibbonNavValidation: return TEXT("RibbonNavValidation");
            case ERouteFailureStage::CompiledPathProjection: return TEXT("CompiledPathProjection");
            case ERouteFailureStage::StartProjection: return TEXT("StartProjection");
            case ERouteFailureStage::GoalProjection: return TEXT("GoalProjection");
            case ERouteFailureStage::TerminalMismatch: return TEXT("TerminalMismatch");
            case ERouteFailureStage::FullPathValidation: return TEXT("FullPathValidation");
            case ERouteFailureStage::NotResolved:
            default:
                return TEXT("NotResolved");
        }
    }

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

        if (NavSys == nullptr
            || NavData == nullptr
            || NOT NavData->HasValidNavmesh())
        { return Result; }

        if (NOT NavData->GetDefaultQueryFilter().IsValid())
        {
            Result._Outcome = EOffPathResolve::PathFailed;
            return Result;
        }

        const auto QueryFilter = NavData->GetDefaultQueryFilter();
        const auto EndpointProjectionExtent = FVector{
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionVerticalExtentCm};
        auto ProjectedFrom = FNavLocation{};
        auto ProjectedTo = FNavLocation{};
        const auto EndpointsProjected =
            NavData->ProjectPoint(
                InFrom,
                ProjectedFrom,
                EndpointProjectionExtent,
                QueryFilter)
            && NavData->ProjectPoint(
                InTo,
                ProjectedTo,
                EndpointProjectionExtent,
                QueryFilter);
        if (NOT EndpointsProjected)
        {
            Result._Outcome = EOffPathResolve::PathFailed;
            return Result;
        }

        Result._Waypoints =
            {ProjectedFrom.Location, ProjectedTo.Location};
        Result._Length = static_cast<float>(
            FVector::Dist(
                ProjectedFrom.Location,
                ProjectedTo.Location));

        auto NavResult = FCk_Nav_PathResult{};

        constexpr auto AllowPartial = false;
        constexpr auto AgentRadiusForFirstSkip = 0.0f;

        const auto FoundPath = FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            ProjectedFrom.Location,
            ProjectedTo.Location,
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
    Validate_ResolvedOffPathLeg(
        UWorld* InWorld,
        const FVector& InFrom,
        const FVector& InTo,
        FOffPathLegResolution& InOutResolution) -> bool
    {
        if (InOutResolution._Outcome != EOffPathResolve::Resolved)
        { return InOutResolution._Outcome != EOffPathResolve::PathFailed; }

        auto ConnectedWaypoints = TArray<FVector>{};
        Append_CompiledWaypoint(ConnectedWaypoints, InFrom);
        for (const auto& Waypoint : InOutResolution._Waypoints)
        { Append_CompiledWaypoint(ConnectedWaypoints, Waypoint); }
        Append_CompiledWaypoint(ConnectedWaypoints, InTo);

        const auto PathIsValid =
            ConnectedWaypoints.Num() >= 2
            && Try_ProjectPathOntoNavmesh(InWorld, ConnectedWaypoints)
            && Is_NavmeshPathValid(InWorld, ConnectedWaypoints);
        if (NOT PathIsValid)
        {
            InOutResolution._Outcome = EOffPathResolve::PathFailed;
            return false;
        }

        auto Length = 0.0f;
        for (auto Index = 0; Index < ConnectedWaypoints.Num() - 1; ++Index)
        {
            Length += static_cast<float>(
                FVector::Dist(
                    ConnectedWaypoints[Index],
                    ConnectedWaypoints[Index + 1]));
        }

        InOutResolution._Waypoints = MoveTemp(ConnectedWaypoints);
        InOutResolution._Length = Length;
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
        auto FailureStage = ERouteFailureStage::NotResolved;

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
            ck::pathnetwork::Display(
                TEXT("PathNetworkFollower [{}] route to {} failed: [{}] at stage [{}]"),
                InHandle,
                GoalLocation,
                InReason,
                Get_RouteFailureStageName(FailureStage));
        };

        auto Network = ck::IsValid(InRequest.Get_Network()) ? InRequest.Get_Network() : InParams.Get_Network();

        if (ck::Is_NOT_Valid(Network) || NOT UCk_Utils_PathNetwork_UE::Has(Network))
        {
            FailureStage = ERouteFailureStage::NoNetwork;
            FailRoute(ECk_PathNetwork_RouteFailReason::NoNetwork);
            return;
        }

        const auto& GraphFragment = Network.Get<FFragment_PathNetwork_Graph>();
        const auto& BuiltNetwork = GraphFragment.Get_Network();

        if (GraphFragment.Get_Epoch() <= 0 || BuiltNetwork._Edges.IsEmpty())
        {
            FailureStage = ERouteFailureStage::NetworkNotBuilt;
            FailRoute(ECk_PathNetwork_RouteFailReason::NetworkNotBuilt);
            return;
        }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
            TEXT("PathNetworkFollower [{}] has no Transform — cannot resolve a route start location"), InHandle)
        {
            FailureStage = ERouteFailureStage::MissingTransform;
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        const auto StartLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
        const auto CostPolicy = Resolve_RouteCostPolicy(InParams);
        auto Shared = Build_RouteGraphSharedData(
            BuiltNetwork,
            StartLocation,
            GoalLocation,
            CostPolicy);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        const auto MaxRepriceIterations = UCk_Utils_PathNetwork_Settings_UE::Get_MaxRepriceIterations();
        const auto RepriceTolerance = UCk_Utils_PathNetwork_Settings_UE::Get_RepriceTolerance();

        auto AcceptedSpans = TArray<FRouteLegSpan>{};
        auto AcceptedOffPathWaypoints = TMap<int32, TArray<FVector>>{};
        auto AcceptedCost = 0.0f;
        auto RouteFound = false;
        auto PlanningFailureStage = ERouteFailureStage::GraphSearch;

        for (auto Iteration = 0; Iteration <= MaxRepriceIterations; ++Iteration)
        {
            const auto Graph = FRouteGraph{&BuiltNetwork, StartLocation, GoalLocation, CostPolicy, Shared};
            const auto Plan = Search_RouteGraph(
                BuiltNetwork,
                StartLocation,
                GoalLocation,
                CostPolicy,
                Shared,
                SearchIterationCap);
            if (NOT Plan._Succeeded)
            { break; }

            const auto& Spans = Plan._Spans;

            auto OffPathWaypoints = TMap<int32, TArray<FVector>>{};
            auto NeedsReprice = false;
            auto HasBlockedHop = false;

            for (auto SpanIndex = 0; SpanIndex < Spans.Num(); ++SpanIndex)
            {
                const auto& Span = Spans[SpanIndex];

                if (NOT Span._IsOffPath)
                { continue; }

                auto Resolution =
                    Resolve_OffPathLeg(
                        World,
                        Span._FromLocation,
                        Span._ToLocation);
                Validate_ResolvedOffPathLeg(
                    World,
                    Span._FromLocation,
                    Span._ToLocation,
                    Resolution);
                const bool IsStrictNetworkGap =
                    Graph.Get_IsComponentTransferHop(
                        Span._FromId,
                        Span._ToId)
                    || Graph.Get_IsLocalNetworkShortcutHop(
                        Span._FromId,
                        Span._ToId);
                if (Resolution._Outcome == EOffPathResolve::NoNavmesh
                    && IsStrictNetworkGap)
                {
                    // Legacy endpoint and direct hops retain their no-navmesh
                    // straight-line fallback. Opt-in network transfers and local
                    // shortcuts are intentionally stricter: an arbitrary gap is
                    // never made traversable without navmesh evidence.
                    Resolution._Outcome = EOffPathResolve::PathFailed;
                }
                OffPathWaypoints.Add(SpanIndex, Resolution._Waypoints);

                const auto Key = FRouteGraph::PackOffPathKey(Span._FromId, Span._ToId);
                const auto Euclidean = static_cast<float>(FVector::Dist(Span._FromLocation, Span._ToLocation));

                switch (Resolution._Outcome)
                {
                    case EOffPathResolve::PathFailed:
                    {
                        HasBlockedHop = true;
                        if (IsStrictNetworkGap)
                        {
                            // Once a strict network gap has failed navmesh
                            // validation, do not let the retry bypass the same
                            // obstacle through the legacy raw direct fallback.
                            Shared->_AllowDirectStartToGoal = false;
                        }
                        if (Iteration < MaxRepriceIterations)
                        {
                            const auto PriceOutBlockedHop =
                                TNumericLimits<float>::Max() / 8.0f;
                            Shared->_RepricedOffPathCosts.Add(
                                Key,
                                PriceOutBlockedHop);
                            NeedsReprice = true;
                        }
                        break;
                    }
                    case EOffPathResolve::Resolved:
                    {
                        if (Iteration < MaxRepriceIterations
                            && Resolution._Length
                                > Euclidean * RepriceTolerance)
                        {
                            Shared->_RepricedOffPathCosts.Add(
                                Key,
                                Graph.Get_OffPathCostForResolvedLength(
                                    Span._FromId,
                                    Span._ToId,
                                    Resolution._Length));
                            NeedsReprice = true;
                        }
                        break;
                    }
                    case EOffPathResolve::NoNavmesh:
                    default:
                        break;
                }
            }

            if (HasBlockedHop)
            {
                if (Iteration >= MaxRepriceIterations)
                {
                    PlanningFailureStage =
                        ERouteFailureStage::OffPathNavValidation;
                    break;
                }
                continue;
            }

            if (NOT NeedsReprice || Iteration >= MaxRepriceIterations)
            {
                AcceptedSpans = Spans;
                AcceptedOffPathWaypoints = MoveTemp(OffPathWaypoints);
                AcceptedCost = Plan._EstimatedCost;
                RouteFound = true;
                break;
            }
        }

        if (NOT RouteFound)
        {
            FailureStage = PlanningFailureStage;
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
                auto LastCompileFailure = ERouteFailureStage::RibbonCompileEmpty;
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
                    {
                        LastCompileFailure = ERouteFailureStage::RibbonCompileEmpty;
                        continue;
                    }

                    if (NOT Try_ProjectPathOntoNavmesh(World, CandidateWaypoints))
                    {
                        LastCompileFailure = ERouteFailureStage::RibbonProjection;
                        continue;
                    }

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
                    {
                        LastCompileFailure = ERouteFailureStage::RibbonContainment;
                        continue;
                    }

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
                    LastCompileFailure = ERouteFailureStage::RibbonNavValidation;
                }

                if (RunWaypoints.Num() < 2)
                {
                    FailureStage = LastCompileFailure;
                    FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                    return;
                }

                for (const auto& Waypoint : RunWaypoints)
                { Append_CompiledWaypoint(Result._CompiledWaypoints, Waypoint); }
            }
        }

        if (NOT Try_ProjectPathOntoNavmesh(World, Result._CompiledWaypoints))
        {
            FailureStage = ERouteFailureStage::CompiledPathProjection;
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        auto NormalizedStart = TArray<FVector>{StartLocation};
        if (NOT Try_ProjectPathOntoNavmesh(World, NormalizedStart))
        {
            FailureStage = ERouteFailureStage::StartProjection;
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
            FailureStage = ERouteFailureStage::GoalProjection;
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }

        const auto HasExactTerminal =
            Result._CompiledWaypoints.Num() > 0 &&
            FVector::Dist(Result._CompiledWaypoints.Last(), NormalizedGoal[0]) <
                CompiledWaypointMergeDistance;
        if (NOT HasExactTerminal)
        {
            FailureStage = ERouteFailureStage::TerminalMismatch;
            FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
            return;
        }
        Result._CompiledWaypoints.Last() = NormalizedGoal[0];

        auto FullMovementPath = Result._CompiledWaypoints;
        FullMovementPath.Insert(NormalizedStart[0], 0);
        if (NOT Is_NavmeshPathValid(World, FullMovementPath))
        {
            FailureStage = ERouteFailureStage::FullPathValidation;
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
        const auto NearEndpointMultiplier = Tuning.Get_NearEndpointCostMultiplier();
        const auto NetworkGapMultiplier = Tuning.Get_NetworkGapCostMultiplier();
        const auto JoinMaxDistance = Tuning.Get_EndpointJoinMaxDistance();
        const auto TransferMaxDistance = Tuning.Get_ComponentTransferMaxDistance();
        const auto LocalShortcutMaxDistance =
            Tuning.Get_LocalNetworkShortcutMaxDistance();
        const auto DirectGraceDistance = Tuning.Get_DirectTripGraceDistance();
        const auto DirectMinimumSavings =
            Tuning.Get_DirectRouteMinimumSavingsFraction();
        const auto SideKeeping = Tuning.Get_SideKeepingFraction();
        const auto Spacing = Tuning.Get_CorridorWaypointSpacing();
        const auto Smoothing = Tuning.Get_CornerSmoothingDistance();
        const auto Clearance = Tuning.Get_DesiredNavmeshClearance();
        const bool TuningIsValid = FMath::IsFinite(Multiplier) && Multiplier >= 1.0f
            && FMath::IsFinite(NearEndpointMultiplier)
            && (NearEndpointMultiplier == 0.0f || NearEndpointMultiplier >= 1.0f)
            && FMath::IsFinite(NetworkGapMultiplier)
            && (NetworkGapMultiplier == 0.0f || NetworkGapMultiplier >= 1.0f)
            && FMath::IsFinite(JoinMaxDistance) && JoinMaxDistance >= 0.0f
            && FMath::IsFinite(TransferMaxDistance) && TransferMaxDistance >= 0.0f
            && FMath::IsFinite(LocalShortcutMaxDistance)
            && LocalShortcutMaxDistance >= 0.0f
            && FMath::IsFinite(DirectGraceDistance) && DirectGraceDistance >= 0.0f
            && FMath::IsFinite(DirectMinimumSavings)
            && DirectMinimumSavings >= 0.0f
            && DirectMinimumSavings <= 1.0f
            && FMath::IsFinite(SideKeeping) && SideKeeping >= 0.0f && SideKeeping <= 0.9f
            && FMath::IsFinite(Spacing) && Spacing >= 50.0f
            && FMath::IsFinite(Smoothing) && Smoothing >= 0.0f && Smoothing <= 1000.0f
            && FMath::IsFinite(Clearance) && Clearance >= 0.0f && Clearance <= 1000.0f;
        CK_ENSURE_IF_NOT(TuningIsValid,
            TEXT("PathNetworkFollower tuning request contains invalid values "
                 "(far/direct multiplier [{}], near multiplier [{}], network gap multiplier [{}], join max [{}], transfer max [{}], local shortcut max [{}], direct grace [{}], minimum direct savings [{}], "
                 "side [{}], spacing [{}], smoothing [{}], clearance [{}])"),
            Multiplier, NearEndpointMultiplier, NetworkGapMultiplier, JoinMaxDistance, TransferMaxDistance,
            LocalShortcutMaxDistance, DirectGraceDistance,
            DirectMinimumSavings,
            SideKeeping, Spacing, Smoothing, Clearance)
        {}
        if (NOT TuningIsValid)
        { return; }

        InParams.Set_OffPathCostMultiplier(Multiplier);
        InParams.Set_NearEndpointCostMultiplier(NearEndpointMultiplier);
        InParams.Set_NetworkGapCostMultiplier(NetworkGapMultiplier);
        InParams.Set_EndpointJoinMaxDistance(JoinMaxDistance);
        InParams.Set_ComponentTransferMaxDistance(TransferMaxDistance);
        InParams.Set_LocalNetworkShortcutMaxDistance(
            LocalShortcutMaxDistance);
        InParams.Set_DirectTripGraceDistance(DirectGraceDistance);
        InParams.Set_DirectRouteMinimumSavingsFraction(
            DirectMinimumSavings);
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
