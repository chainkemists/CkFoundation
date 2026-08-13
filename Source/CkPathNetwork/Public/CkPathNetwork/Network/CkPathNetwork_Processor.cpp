#include "CkPathNetwork_Processor.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/CkPathNetwork_Stats.h"
#include "CkPathNetwork/Network/CkPathNetwork_Build.h"
#include "CkPathNetwork/Network/CkPathNetwork_CorridorCompile.h"
#include "CkPathNetwork/Network/CkPathNetwork_PathSimplify.h"
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
#include <NavFilters/NavigationQueryFilter.h>

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
DECLARE_CYCLE_STAT(
    TEXT("PathNetwork::FollowerHandleRequests"),
    STAT_CkPathNetwork_FollowerHandleRequests,
    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::PlanRoute"),        STAT_CkPathNetwork_PlanRoute,        STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::GraphSearch"),      STAT_CkPathNetwork_GraphSearch,      STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::OffPathNav"),       STAT_CkPathNetwork_OffPathNav,       STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::CorridorCompile"),  STAT_CkPathNetwork_CorridorCompile,  STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::FinalNavValidate"), STAT_CkPathNetwork_FinalNavValidate, STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::RibbonCompile"),    STAT_CkPathNetwork_RibbonCompile,    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::RibbonProject"),    STAT_CkPathNetwork_RibbonProject,    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(
    TEXT("PathNetwork::RibbonProjContain"),
    STAT_CkPathNetwork_RibbonProjContain,
    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::RibbonClearance"),  STAT_CkPathNetwork_RibbonClearance,  STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::RibbonResolve"),    STAT_CkPathNetwork_RibbonResolve,    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(
    TEXT("PathNetwork::RibbonResolveContain"),
    STAT_CkPathNetwork_RibbonResolveContain,
    STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::FinalEndpoints"),   STAT_CkPathNetwork_FinalEndpoints,   STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::FinalResolve"),     STAT_CkPathNetwork_FinalResolve,     STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::RouteSignal"),      STAT_CkPathNetwork_RouteSignal,      STATGROUP_CkPathNetwork);
DECLARE_CYCLE_STAT(TEXT("PathNetwork::Invalidate"),       STAT_CkPathNetwork_Invalidate,       STATGROUP_CkPathNetwork);
DECLARE_DWORD_COUNTER_STAT(TEXT("Static Cache Hits"),     STAT_CkPathNetwork_StaticCacheHits,   STATGROUP_CkPathNetwork);
DECLARE_DWORD_COUNTER_STAT(TEXT("Static Cache Misses"),   STAT_CkPathNetwork_StaticCacheMisses, STATGROUP_CkPathNetwork);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_processor
{
    using namespace ck::pathnetwork;

    constexpr auto SearchIterationCap = 200000;
    constexpr auto MaxRouteStaticDataCacheEntries = 8;
    constexpr auto CompiledWaypointMergeDistance = 1.0f;
    constexpr auto ClearanceProjectionPlanarExtentCm = 25.0f;
    constexpr auto RibbonProjectionPlanarExtentCm = 50.0f;
    constexpr auto ClearanceProjectionVerticalExtentCm = 100.0f;
    constexpr auto ClearanceImprovementEpsilonCm = 1.0f;
    constexpr auto OffPathSimplificationMaximumVerticalDeviationCm = 10.0f;

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

    auto
    Get_RouteNodeKindName(
        ERouteNodeKind InKind)
        -> const TCHAR*
    {
        switch (InKind)
        {
            case ERouteNodeKind::Start: return TEXT("Start");
            case ERouteNodeKind::Goal: return TEXT("Goal");
            case ERouteNodeKind::NetNode: return TEXT("NetNode");
            case ERouteNodeKind::OverlayPoint: return TEXT("OverlayPoint");
            default: return TEXT("Unknown");
        }
    }

    struct FOffPathLegResolution
    {
        TArray<FVector> _Waypoints;
        float _Length = 0.0f;
        EOffPathResolve _Outcome = EOffPathResolve::NoNavmesh;
    };

    auto
    Resolve_QueryFilter(
        const ARecastNavMesh& InNavData,
        TSubclassOf<UNavigationQueryFilter> InFilterClass) -> FSharedConstNavQueryFilter
    {
        auto QueryFilter = InNavData.GetDefaultQueryFilter();
        if (InFilterClass.Get() != nullptr)
        { QueryFilter = UNavigationQueryFilter::GetQueryFilter(InNavData, InFilterClass); }
        return QueryFilter;
    }

    // Direct FindPathSync (bypassing the CkNavigation request path) is safe here: every caller drains
    // under the route processor's per-frame budget (_MaxRouteQueriesPerFrame).
    auto
    Resolve_OffPathLeg(
        UWorld* InWorld,
        const FVector& InFrom,
        const FVector& InTo,
        bool InFromIsRouteEndpoint,
        bool InToIsRouteEndpoint,
        TSubclassOf<UNavigationQueryFilter> InFilterClass) -> FOffPathLegResolution
    {
        auto Result = FOffPathLegResolution{};
        Result._Waypoints = {InFrom, InTo};
        Result._Length = static_cast<float>(FVector::Dist(InFrom, InTo));

        auto* NavSys = IsValid(InWorld) ? UNavigationSystemV1::GetCurrent(InWorld) : nullptr;
        auto* NavData = (NavSys != nullptr)
            ? Cast<ARecastNavMesh>(NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
            : nullptr;
        const auto HasValidNavmesh =
            NavData != nullptr
            && NavData->HasValidNavmesh();

        if (NavSys == nullptr
            || NavData == nullptr
            || NOT HasValidNavmesh)
        {
            ck::pathnetwork::Verbose(
                TEXT("[PNDiag] off-path nav unavailable: raw [{}] -> [{}], "
                     "world [{}], navSystem [{}], navData [{}], validNavmesh [{}]"),
                InFrom,
                InTo,
                IsValid(InWorld),
                NavSys != nullptr,
                NavData != nullptr,
                HasValidNavmesh);
            return Result;
        }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        {
            ck::pathnetwork::Verbose(
                TEXT("[PNDiag] off-path query rejected: raw [{}] -> [{}], "
                     "query filter invalid, allowPartial [false]"),
                InFrom,
                InTo);
            Result._Outcome = EOffPathResolve::PathFailed;
            return Result;
        }

        const auto StrictProjectionExtent = FVector{
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionVerticalExtentCm};
        const auto NavQueryProjectionExtent =
            UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec();
        const auto FromProjectionExtent = InFromIsRouteEndpoint
            ? NavQueryProjectionExtent
            : StrictProjectionExtent;
        const auto ToProjectionExtent = InToIsRouteEndpoint
            ? NavQueryProjectionExtent
            : StrictProjectionExtent;
        auto ProjectedFrom = FNavLocation{};
        auto ProjectedTo = FNavLocation{};
        const auto FromProjected =
            NavData->ProjectPoint(
                InFrom,
                ProjectedFrom,
                FromProjectionExtent,
                QueryFilter);
        const auto ToProjected =
            FromProjected
            && NavData->ProjectPoint(
                InTo,
                ProjectedTo,
                ToProjectionExtent,
                QueryFilter);
        if (NOT FromProjected || NOT ToProjected)
        {
            ck::pathnetwork::Verbose(
                TEXT("[PNDiag] off-path endpoint projection failed: raw [{}] -> [{}], "
                     "projected [{}:{}] -> [{}:{}], endpointExtents [{}] -> [{}], "
                     "allowPartial [false]"),
                InFrom,
                InTo,
                FromProjected,
                ProjectedFrom.Location,
                ToProjected,
                ProjectedTo.Location,
                FromProjectionExtent,
                ToProjectionExtent);
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
        const auto CornerOffsetDistance = NavData->GetConfig().AgentRadius;

        const auto FoundPath = FCk_Nav_Algorithm::FindPathSync(
            *NavSys,
            *NavData,
            ProjectedFrom.Location,
            ProjectedTo.Location,
            AllowPartial,
            UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent(),
            UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent(),
            AgentRadiusForFirstSkip,
            NavResult,
            InFilterClass,
            CornerOffsetDistance);

        if (NOT FoundPath)
        {
            const auto& Diagnostics = NavResult.Get_Diagnostics();
            ck::pathnetwork::Verbose(
                TEXT("[PNDiag] off-path FindPathSync failed: raw [{}] -> [{}], "
                     "endpointProjected [{}] -> [{}], navProjected [{}:{}] -> [{}:{}], "
                     "endpointExtents [{}] -> [{}], queryExtent [xy={}, z={}], "
                     "allowPartial [false], status [{}], reason [{}]"),
                InFrom,
                InTo,
                ProjectedFrom.Location,
                ProjectedTo.Location,
                Diagnostics.Get_StartProjected(),
                Diagnostics.Get_LastProjectedStart(),
                Diagnostics.Get_EndProjected(),
                Diagnostics.Get_LastProjectedEnd(),
                FromProjectionExtent,
                ToProjectionExtent,
                UCk_Utils_Nav_Settings_UE::Get_NavQuerySearchHalfExtent(),
                UCk_Utils_Nav_Settings_UE::Get_NavQueryVerticalHalfExtent(),
                NavResult.Get_Status(),
                Diagnostics.Get_LastFailReason());
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
    Is_NavmeshSegmentDirectlyWalkable(
        const ARecastNavMesh& InNavData,
        const FSharedConstNavQueryFilter& InQueryFilter,
        const FVector& InFrom,
        const FVector& InTo) -> bool
    {
        auto HitLocation = FVector::ZeroVector;
        auto RaycastResult = ARecastNavMesh::FRaycastResult{};
        return NOT ARecastNavMesh::NavMeshRaycast(
            &InNavData,
            InFrom,
            InTo,
            HitLocation,
            InQueryFilter,
            nullptr,
            RaycastResult);
    }

    auto
    Try_ResolveNavmeshSegment(
        UNavigationSystemV1& InNavSys,
        ARecastNavMesh& InNavData,
        const FSharedConstNavQueryFilter& InQueryFilter,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        const FVector& InFrom,
        const FVector& InTo,
        float InMaxCornerOffsetCm,
        TArray<FVector>& OutWaypoints) -> bool
    {
        OutWaypoints.Reset();
        Append_CompiledWaypoint(OutWaypoints, InFrom);
        Append_CompiledWaypoint(OutWaypoints, InTo);

        if (FVector::DistSquared(InFrom, InTo) <= FMath::Square(CompiledWaypointMergeDistance))
        { return true; }

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
        if (NOT HitNavmeshBoundary)
        { return true; }

        // A raycast only answers whether this exact Detour corridor stays unobstructed. Points on
        // shared polygon seams can start in the wrong containing polygon, and genuine local
        // obstacles may have a short valid detour. Ask Unreal for that route and publish its actual
        // waypoints instead of rejecting a path Unreal can walk.
        auto DetourResult = FCk_Nav_PathResult{};
        constexpr auto AllowPartial = false;
        constexpr auto AgentRadiusForFirstSkip = 0.0f;
        // An offset corner can leave the ribbon by up to the offset distance, so callers whose
        // waypoints face a downstream Is_SegmentInsideRibbonRun check cap it at their slack.
        const auto CornerOffsetDistance = FMath::Min(
            InNavData.GetConfig().AgentRadius,
            InMaxCornerOffsetCm);
        const auto DetourFound = FCk_Nav_Algorithm::FindPathSync(
            InNavSys,
            InNavData,
            InFrom,
            InTo,
            AllowPartial,
            ClearanceProjectionPlanarExtentCm,
            ClearanceProjectionVerticalExtentCm,
            AgentRadiusForFirstSkip,
            DetourResult,
            InFilterClass,
            CornerOffsetDistance);
        if (NOT DetourFound || DetourResult.Get_Status() != ECk_Nav_PathStatus::Ready)
        { return false; }

        const auto& DetourWaypoints = DetourResult.Get_Waypoints();
        if (DetourWaypoints.IsEmpty())
        { return false; }

        OutWaypoints.Reset(DetourWaypoints.Num());
        for (const auto& Waypoint : DetourWaypoints)
        { Append_CompiledWaypoint(OutWaypoints, Waypoint); }

        ck::pathnetwork::Verbose(
            TEXT("[PNDiag] nav raycast boundary recovered by strict path: [{}] -> [{}], "
                 "hit [{}] at [{}], endInCorridor [{}], waypoints [{}]"),
            InFrom,
            InTo,
            RaycastResult.HitTime,
            HitLocation,
            static_cast<bool>(RaycastResult.bIsRaycastEndInCorridor),
            OutWaypoints.Num());
        return NOT OutWaypoints.IsEmpty();
    }

    auto
    Try_ProjectPathOntoNavmesh(
        UWorld* InWorld,
        TArray<FVector>& InOutWaypoints,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        float InPlanarExtentCm) -> bool
    {
        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return true; }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        { return false; }
        const auto ProjectionExtent = FVector{
            InPlanarExtentCm,
            InPlanarExtentCm,
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
                    InPlanarExtentCm ||
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
    Try_ProjectRouteEndpointOntoNavmesh(
        UWorld* InWorld,
        const FVector& InEndpoint,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        FVector& OutProjectedEndpoint) -> bool
    {
        OutProjectedEndpoint = InEndpoint;

        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return true; }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        { return false; }

        auto ProjectedEndpoint = FNavLocation{};
        if (NOT NavData->ProjectPoint(
                InEndpoint,
                ProjectedEndpoint,
                UCk_Utils_Nav_Settings_UE::Get_NavQueryProjectionExtentVec(),
                QueryFilter))
        { return false; }

        OutProjectedEndpoint = ProjectedEndpoint.Location;
        return true;
    }

    auto
    Try_ResolvePathOntoNavmesh(
        UWorld* InWorld,
        TArray<FVector>& InOutWaypoints,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        float InMaxCornerOffsetCm) -> bool
    {
        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return true; }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        { return false; }

        auto* NavSys = UNavigationSystemV1::GetCurrent(InWorld);
        if (NavSys == nullptr || InOutWaypoints.Num() < 2)
        { return NavSys != nullptr; }

        auto ResolvedWaypoints = TArray<FVector>{};
        ResolvedWaypoints.Reserve(InOutWaypoints.Num());
        Append_CompiledWaypoint(ResolvedWaypoints, InOutWaypoints[0]);

        for (auto Index = 0; Index < InOutWaypoints.Num() - 1; ++Index)
        {
            auto SegmentWaypoints = TArray<FVector>{};
            if (NOT Try_ResolveNavmeshSegment(
                    *NavSys,
                    *NavData,
                    QueryFilter,
                    InFilterClass,
                    InOutWaypoints[Index],
                    InOutWaypoints[Index + 1],
                    InMaxCornerOffsetCm,
                    SegmentWaypoints))
            { return false; }

            for (const auto& Waypoint : SegmentWaypoints)
            { Append_CompiledWaypoint(ResolvedWaypoints, Waypoint); }
        }

        InOutWaypoints = MoveTemp(ResolvedWaypoints);
        return NOT InOutWaypoints.IsEmpty();
    }

    enum class EConstrainedPathResolution : uint8
    {
        Succeeded,
        NavFailed,
        RibbonContainmentFailed
    };

    auto
    Try_ResolvePathOntoNavmeshWithRibbonConstraints(
        UWorld* InWorld,
        TArray<FVector>& InOutWaypoints,
        TConstArrayView<int32> InSegmentRibbonRunIndices,
        TConstArrayView<bool> InSegmentNeedsValidation,
        const FBuiltNetwork& InNetwork,
        TConstArrayView<TArray<FRouteLegSpan>> InRibbonRuns,
        float InRibbonTolerance,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        FRibbonContainmentFailure& OutContainmentFailure,
        int32& OutOriginalSegmentIndex,
        int32& OutRibbonRunIndex) -> EConstrainedPathResolution
    {
        OutContainmentFailure = FRibbonContainmentFailure{};
        OutOriginalSegmentIndex = INDEX_NONE;
        OutRibbonRunIndex = INDEX_NONE;

        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return EConstrainedPathResolution::Succeeded; }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        { return EConstrainedPathResolution::NavFailed; }

        auto* NavSys = UNavigationSystemV1::GetCurrent(InWorld);
        if (NavSys == nullptr || InOutWaypoints.Num() < 2 ||
            InSegmentRibbonRunIndices.Num() != InOutWaypoints.Num() - 1 ||
            InSegmentNeedsValidation.Num() != InOutWaypoints.Num() - 1)
        { return EConstrainedPathResolution::NavFailed; }

        auto ResolvedWaypoints = TArray<FVector>{};
        ResolvedWaypoints.Reserve(InOutWaypoints.Num());
        Append_CompiledWaypoint(ResolvedWaypoints, InOutWaypoints[0]);

        const auto MaxCornerOffsetCm =
            FMath::Max(0.0f, InRibbonTolerance - RibbonContainmentToleranceCm);
        for (auto Index = 0; Index < InOutWaypoints.Num() - 1; ++Index)
        {
            auto SegmentWaypoints = TArray<FVector>{};
            const auto NeedsValidation = InSegmentNeedsValidation[Index];
            if (NeedsValidation)
            {
                if (NOT Try_ResolveNavmeshSegment(
                        *NavSys,
                        *NavData,
                        QueryFilter,
                        InFilterClass,
                        InOutWaypoints[Index],
                        InOutWaypoints[Index + 1],
                        MaxCornerOffsetCm,
                        SegmentWaypoints))
                { return EConstrainedPathResolution::NavFailed; }
            }
            else
            {
                // Each source leg was projected and nav-resolved before assembly. Preserve those
                // already-proven segments verbatim; only seams and normalized endpoints need a
                // second proof after assembly changes their adjacency.
                Append_CompiledWaypoint(SegmentWaypoints, InOutWaypoints[Index]);
                Append_CompiledWaypoint(SegmentWaypoints, InOutWaypoints[Index + 1]);
            }

            const auto RibbonRunIndex = InSegmentRibbonRunIndices[Index];
            if (RibbonRunIndex != INDEX_NONE)
            {
                if (NOT InRibbonRuns.IsValidIndex(RibbonRunIndex))
                { return EConstrainedPathResolution::NavFailed; }

                if (NeedsValidation &&
                    NOT Is_PathInsideRibbonRun(
                        InNetwork,
                        InRibbonRuns[RibbonRunIndex],
                        SegmentWaypoints,
                        RibbonContainmentSampleSpacingCm,
                        InRibbonTolerance,
                        &OutContainmentFailure))
                {
                    OutOriginalSegmentIndex = Index;
                    OutRibbonRunIndex = RibbonRunIndex;
                    return EConstrainedPathResolution::RibbonContainmentFailed;
                }
            }

            for (const auto& Waypoint : SegmentWaypoints)
            { Append_CompiledWaypoint(ResolvedWaypoints, Waypoint); }
        }

        InOutWaypoints = MoveTemp(ResolvedWaypoints);
        return InOutWaypoints.IsEmpty()
            ? EConstrainedPathResolution::NavFailed
            : EConstrainedPathResolution::Succeeded;
    }

    auto
    Validate_ResolvedOffPathLeg(
        UWorld* InWorld,
        const FVector& InFrom,
        const FVector& InTo,
        bool InFromIsRouteEndpoint,
        bool InToIsRouteEndpoint,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        FOffPathLegResolution& InOutResolution) -> bool
    {
        if (InOutResolution._Outcome != EOffPathResolve::Resolved)
        { return InOutResolution._Outcome != EOffPathResolve::PathFailed; }

        auto ConnectedWaypoints = TArray<FVector>{};
        if (NOT InFromIsRouteEndpoint)
        { Append_CompiledWaypoint(ConnectedWaypoints, InFrom); }
        for (const auto& Waypoint : InOutResolution._Waypoints)
        { Append_CompiledWaypoint(ConnectedWaypoints, Waypoint); }
        if (NOT InToIsRouteEndpoint)
        { Append_CompiledWaypoint(ConnectedWaypoints, InTo); }

        // Off-path connectors are never ribbon-contained, so corner offsetting stays unbounded.
        constexpr auto UnboundedCornerOffsetCm = TNumericLimits<float>::Max();
        const auto PathIsValid =
            ConnectedWaypoints.Num() >= 2
            && Try_ProjectPathOntoNavmesh(
                InWorld,
                ConnectedWaypoints,
                InFilterClass,
                ClearanceProjectionPlanarExtentCm)
            && Try_ResolvePathOntoNavmesh(
                InWorld,
                ConnectedWaypoints,
                InFilterClass,
                UnboundedCornerOffsetCm);
        if (NOT PathIsValid)
        {
            InOutResolution._Outcome = EOffPathResolve::PathFailed;
            return false;
        }

        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData != nullptr && NavData->HasValidNavmesh())
        {
            const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
            if (QueryFilter.IsValid())
            {
                const auto OriginalWaypointCount = ConnectedWaypoints.Num();
                ConnectedWaypoints = Simplify_PathByTraversal(
                    ConnectedWaypoints,
                    [&](const FVector& InShortcutFrom, const FVector& InShortcutTo)
                    {
                        return Is_NavmeshSegmentDirectlyWalkable(
                            *NavData,
                            QueryFilter,
                            InShortcutFrom,
                            InShortcutTo);
                    },
                    OffPathSimplificationMaximumVerticalDeviationCm);

                if (ConnectedWaypoints.Num() < OriginalWaypointCount)
                {
                    ck::pathnetwork::Verbose(
                        TEXT("[PNDiag] off-path connector removed [{}] redundant nav waypoints"),
                        OriginalWaypointCount - ConnectedWaypoints.Num());
                }
            }
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
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        TArray<FVector>& InOutWaypoints) -> void
    {
        if (InDesiredClearance <= UE_KINDA_SMALL_NUMBER || InOutWaypoints.Num() < 3)
        { return; }

        auto* NavData = Get_DefaultRecastNavmesh(InWorld);
        if (NavData == nullptr || NOT NavData->HasValidNavmesh())
        { return; }

        const auto QueryFilter = Resolve_QueryFilter(*NavData, InFilterClass);
        if (NOT QueryFilter.IsValid())
        { return; }
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

                if (NOT Is_PointInsideRibbonRun(
                        InNetwork,
                        InSpans,
                        Candidate,
                        RibbonContainmentToleranceCm) ||
                    NOT Is_SegmentInsideRibbonRun(
                        InNetwork,
                        InSpans,
                        InOutWaypoints[WaypointIndex - 1],
                        Candidate,
                        RibbonContainmentSampleSpacingCm,
                        RibbonContainmentToleranceCm) ||
                    NOT Is_SegmentInsideRibbonRun(
                        InNetwork,
                        InSpans,
                        Candidate,
                        InOutWaypoints[WaypointIndex + 1],
                        RibbonContainmentSampleSpacingCm,
                        RibbonContainmentToleranceCm))
                { continue; }

                if (NOT Is_NavmeshSegmentDirectlyWalkable(
                        *NavData,
                        QueryFilter,
                        InOutWaypoints[WaypointIndex - 1],
                        Candidate) ||
                    NOT Is_NavmeshSegmentDirectlyWalkable(
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
        InGraph._RouteGraphStaticDataByPolicy.Reset();
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
        InGraph._RouteGraphStaticDataByPolicy.Reset();
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
        SCOPE_CYCLE_COUNTER(
            STAT_CkPathNetwork_FollowerHandleRequests);

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

        // Request_FindRoute records the most recent caller-owned revision before this
        // deferred work can run. An older same-goal request must neither replace that
        // pending/result state nor emit an obsolete route signal.
        if (InRequest.Get_RequestRevision() != InCorridor._Result.Get_RequestRevision())
        { return; }

        const auto GoalLocation = InRequest.Get_GoalLocation();
        const auto FilterClass = UCk_Utils_Nav_Settings_UE::Get_QueryFilterClass(
            InRequest.Get_NavQueryFilter());
        auto FailureStage = ERouteFailureStage::NotResolved;

        const auto FailRoute = [&](ECk_PathNetwork_RouteFailReason InReason)
        {
            InCorridor._Result = FCk_PathNetwork_RouteResult{};
            InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Failed;
            InCorridor._Result._FailReason = InReason;
            InCorridor._Result._GoalLocation = GoalLocation;
            InCorridor._Result._TuningRevision = InRequest.Get_TuningRevision();
            InCorridor._Result._RequestRevision = InRequest.Get_RequestRevision();

            auto Follower = InHandle;
            {
                SCOPE_CYCLE_COUNTER(
                    STAT_CkPathNetwork_RouteSignal);
                UUtils_Signal_PathNetworkFollower_OnRouteFailed::Broadcast(Follower, MakePayload(Follower));
            }

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

        auto& GraphFragment = Network.Get<FFragment_PathNetwork_Graph>();
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
        const auto StaticDataKey =
            FRouteGraphStaticDataKey::TryFromPolicy(
                CostPolicy);
        auto StaticData =
            TSharedPtr<
                const FRouteGraphStaticData>{};
        if (StaticDataKey.IsSet())
        {
            if (const auto* CachedStaticData =
                    GraphFragment
                        ._RouteGraphStaticDataByPolicy
                        .Find(StaticDataKey.GetValue()))
            {
                StaticData = *CachedStaticData;
                INC_DWORD_STAT(
                    STAT_CkPathNetwork_StaticCacheHits);
            }
            else
            {
                INC_DWORD_STAT(
                    STAT_CkPathNetwork_StaticCacheMisses);
                StaticData =
                    Build_RouteGraphStaticData(
                        BuiltNetwork,
                        CostPolicy);

                // Runtime tuning changes may introduce distinct construction
                // policies. Keep the per-network cache bounded; clearing at
                // the cap preserves correctness and still favors the
                // overwhelmingly common shared-policy crowd.
                if (GraphFragment
                        ._RouteGraphStaticDataByPolicy
                        .Num()
                    >= MaxRouteStaticDataCacheEntries)
                {
                    GraphFragment
                        ._RouteGraphStaticDataByPolicy
                        .Reset();
                }
                GraphFragment
                    ._RouteGraphStaticDataByPolicy
                    .Add(
                        StaticDataKey.GetValue(),
                        StaticData);
            }
        }
        else
        {
            StaticData = Build_RouteGraphStaticData(
                BuiltNetwork,
                CostPolicy);
        }
        auto Shared = Build_RouteGraphSharedData(
            BuiltNetwork,
            StartLocation,
            GoalLocation,
            CostPolicy,
            StaticData);

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
            auto Plan = FRoutePlanResult{};
            {
                SCOPE_CYCLE_COUNTER(
                    STAT_CkPathNetwork_GraphSearch);
                Plan = Search_RouteGraph(
                    BuiltNetwork,
                    StartLocation,
                    GoalLocation,
                    CostPolicy,
                    Shared,
                    SearchIterationCap);
            }
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

                const auto IsComponentTransfer =
                    Graph.Get_IsComponentTransferHop(
                        Span._FromId,
                        Span._ToId);
                const auto IsLocalNetworkShortcut =
                    Graph.Get_IsLocalNetworkShortcutHop(
                        Span._FromId,
                        Span._ToId);
                const auto IsStrictNetworkGap =
                    IsComponentTransfer
                    || IsLocalNetworkShortcut;
                const auto IsDirectStartToGoal =
                    Span._FromId._Kind == ERouteNodeKind::Start
                    && Span._ToId._Kind == ERouteNodeKind::Goal;
                const auto FromIsRouteEndpoint =
                    Span._FromId._Kind == ERouteNodeKind::Start;
                const auto ToIsRouteEndpoint =
                    Span._ToId._Kind == ERouteNodeKind::Goal;
                const auto Euclidean = static_cast<float>(
                    FVector::Dist(
                        Span._FromLocation,
                        Span._ToLocation));

                ck::pathnetwork::Verbose(
                    TEXT("[PNDiag] follower [{}] goal [{}] iteration [{}/{}] span [{}] "
                         "[{}:{}] -> [{}:{}], direct [{}], componentTransfer [{}], "
                         "localShortcut [{}], strictGap [{}], raw [{}] -> [{}], "
                         "euclidean [{}], directEnabled [{}]"),
                    InHandle,
                    GoalLocation,
                    Iteration,
                    MaxRepriceIterations,
                    SpanIndex,
                    Get_RouteNodeKindName(Span._FromId._Kind),
                    Span._FromId._Index,
                    Get_RouteNodeKindName(Span._ToId._Kind),
                    Span._ToId._Index,
                    IsDirectStartToGoal,
                    IsComponentTransfer,
                    IsLocalNetworkShortcut,
                    IsStrictNetworkGap,
                    Span._FromLocation,
                    Span._ToLocation,
                    Euclidean,
                    Shared->_AllowDirectStartToGoal);

                auto Resolution =
                    FOffPathLegResolution{};
                {
                    SCOPE_CYCLE_COUNTER(
                        STAT_CkPathNetwork_OffPathNav);
                    Resolution =
                        Resolve_OffPathLeg(
                            World,
                            Span._FromLocation,
                            Span._ToLocation,
                            FromIsRouteEndpoint,
                            ToIsRouteEndpoint,
                            FilterClass);
                    Validate_ResolvedOffPathLeg(
                        World,
                        Span._FromLocation,
                        Span._ToLocation,
                        FromIsRouteEndpoint,
                        ToIsRouteEndpoint,
                        FilterClass,
                        Resolution);
                }
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

                switch (Resolution._Outcome)
                {
                    case EOffPathResolve::PathFailed:
                    {
                        HasBlockedHop = true;

                        if (IsDirectStartToGoal
                            && Iteration == 0
                            && UE_LOG_ACTIVE(CkPathNetwork, Verbose))
                        {
                            const auto StartCandidates =
                                Gather_RouteEndpointCandidates(
                                    BuiltNetwork,
                                    StartLocation,
                                    CostPolicy);
                            const auto GoalCandidates =
                                Gather_RouteEndpointCandidates(
                                    BuiltNetwork,
                                    GoalLocation,
                                    CostPolicy);
                            const auto Topology =
                                Analyze_NetworkTopology(BuiltNetwork);

                            struct FNearestCandidateDiagnostic
                            {
                                int32 _EdgeId = INDEX_NONE;
                                int32 _ComponentId = INDEX_NONE;
                                double _Distance = TNumericLimits<double>::Max();
                            };
                            const auto FindNearestCandidate =
                                [&](const FVector& InEndpoint)
                                    -> FNearestCandidateDiagnostic
                                {
                                    auto Nearest = FNearestCandidateDiagnostic{};
                                    for (auto EdgeId = 0;
                                         EdgeId < BuiltNetwork._Edges.Num();
                                         ++EdgeId)
                                    {
                                        const auto Projection =
                                            BuiltNetwork.Project_OntoEdge(
                                                EdgeId,
                                                InEndpoint);
                                        if (Projection._Distance >= Nearest._Distance)
                                        { continue; }

                                        const auto NodeA =
                                            BuiltNetwork._Edges[EdgeId]._NodeA;
                                        Nearest._EdgeId = EdgeId;
                                        Nearest._ComponentId =
                                            Topology._ComponentByNode.IsValidIndex(NodeA)
                                                ? Topology._ComponentByNode[NodeA]
                                                : INDEX_NONE;
                                        Nearest._Distance = Projection._Distance;
                                    }
                                    return Nearest;
                                };
                            const auto NearestStart =
                                FindNearestCandidate(StartLocation);
                            const auto NearestGoal =
                                FindNearestCandidate(GoalLocation);

                            auto NetworkOnlyShared =
                                MakeShared<FRouteGraphSharedData>(*Shared);
                            NetworkOnlyShared->_AllowDirectStartToGoal = false;
                            const auto NetworkOnlyPlan =
                                Search_RouteGraph(
                                    BuiltNetwork,
                                    StartLocation,
                                    GoalLocation,
                                    CostPolicy,
                                    NetworkOnlyShared,
                                    SearchIterationCap);

                            ck::pathnetwork::Verbose(
                                TEXT("[PNDiag] follower [{}] direct alternative: "
                                     "startCandidates [{}], goalCandidates [{}], "
                                     "joinMaxDistance [{}], components [{}], "
                                     "nearestStart [edge={}, component={}, distance={}], "
                                     "nearestGoal [edge={}, component={}, distance={}], "
                                     "networkOnlySucceeded [{}], "
                                     "networkOnlyOutcome [{}], networkOnlyUsesNetwork [{}], "
                                     "networkOnlyPathNodes [{}], networkOnlyCost [{}]"),
                                InHandle,
                                StartCandidates.Num(),
                                GoalCandidates.Num(),
                                CostPolicy._EndpointJoinMaxDistance,
                                Topology._ComponentCount,
                                NearestStart._EdgeId,
                                NearestStart._ComponentId,
                                NearestStart._Distance,
                                NearestGoal._EdgeId,
                                NearestGoal._ComponentId,
                                NearestGoal._Distance,
                                NetworkOnlyPlan._Succeeded,
                                static_cast<uint8>(
                                    NetworkOnlyPlan._SearchOutcome),
                                Uses_Network(NetworkOnlyPlan),
                                NetworkOnlyPlan._Path.Num(),
                                NetworkOnlyPlan._EstimatedCost);

                            const auto LogCandidates =
                                [&](const TCHAR* InEndpointName,
                                    const FVector& InEndpointLocation,
                                    const TArray<FRouteOverlayPoint>& InCandidates)
                                {
                                    for (auto CandidateIndex = 0;
                                         CandidateIndex < InCandidates.Num();
                                         ++CandidateIndex)
                                    {
                                        const auto& Candidate =
                                            InCandidates[CandidateIndex];
                                        const auto EdgeIsValid =
                                            BuiltNetwork._Edges.IsValidIndex(
                                                Candidate._EdgeId);
                                        const auto NodeA = EdgeIsValid
                                            ? BuiltNetwork
                                                ._Edges[Candidate._EdgeId]
                                                ._NodeA
                                            : INDEX_NONE;
                                        const auto NodeB = EdgeIsValid
                                            ? BuiltNetwork
                                                ._Edges[Candidate._EdgeId]
                                                ._NodeB
                                            : INDEX_NONE;
                                        const auto ComponentId =
                                            Topology
                                                    ._ComponentByNode
                                                    .IsValidIndex(NodeA)
                                                ? Topology
                                                    ._ComponentByNode[NodeA]
                                                : INDEX_NONE;

                                        ck::pathnetwork::Verbose(
                                            TEXT("[PNDiag] follower [{}] {} candidate [{}]: "
                                                 "edge [{}], nodes [{}]->[{}], component [{}], "
                                                 "distance [{}], distAlong [{}], location [{}]"),
                                            InHandle,
                                            InEndpointName,
                                            CandidateIndex,
                                            Candidate._EdgeId,
                                            NodeA,
                                            NodeB,
                                            ComponentId,
                                            FVector::Dist(
                                                InEndpointLocation,
                                                Candidate._Location),
                                            Candidate._DistAlong,
                                            Candidate._Location);
                                    }
                                };
                            LogCandidates(
                                TEXT("start"),
                                StartLocation,
                                StartCandidates);
                            LogCandidates(
                                TEXT("goal"),
                                GoalLocation,
                                GoalCandidates);
                        }

                        const auto DirectWasEnabled =
                            Shared->_AllowDirectStartToGoal;
                        if (IsStrictNetworkGap)
                        {
                            // Once a strict network gap has failed navmesh
                            // validation, do not let the retry bypass the same
                            // obstacle through the legacy raw direct fallback.
                            Shared->_AllowDirectStartToGoal = false;
                        }
                        ck::pathnetwork::Verbose(
                            TEXT("[PNDiag] follower [{}] goal [{}] rejected span [{}] "
                                 "[{}:{}] -> [{}:{}] at iteration [{}]: direct [{}], "
                                 "strictGap [{}], directEnabled [{}] -> [{}], reprice [{}]"),
                            InHandle,
                            GoalLocation,
                            SpanIndex,
                            Get_RouteNodeKindName(Span._FromId._Kind),
                            Span._FromId._Index,
                            Get_RouteNodeKindName(Span._ToId._Kind),
                            Span._ToId._Index,
                            Iteration,
                            IsDirectStartToGoal,
                            IsStrictNetworkGap,
                            DirectWasEnabled,
                            Shared->_AllowDirectStartToGoal,
                            Iteration < MaxRepriceIterations);
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
        Result._RequestRevision = InRequest.Get_RequestRevision();
        auto RibbonRuns = TArray<TArray<FRouteLegSpan>>{};
        auto CompiledSegmentRibbonRunIndices = TArray<int32>{};
        auto CompiledSegmentNeedsValidation = TArray<bool>{};
        const auto AppendCompiledPath =
            [&](TConstArrayView<FVector> InWaypoints, int32 InRibbonRunIndex)
            {
                auto HasPendingSeam = NOT Result._CompiledWaypoints.IsEmpty();
                for (const auto& Waypoint : InWaypoints)
                {
                    const auto PreviousWaypointCount = Result._CompiledWaypoints.Num();
                    Append_CompiledWaypoint(Result._CompiledWaypoints, Waypoint);
                    if (PreviousWaypointCount > 0 &&
                        Result._CompiledWaypoints.Num() > PreviousWaypointCount)
                    {
                        CompiledSegmentRibbonRunIndices.Add(InRibbonRunIndex);
                        CompiledSegmentNeedsValidation.Add(HasPendingSeam);
                        HasPendingSeam = false;
                    }
                }
            };

        {
            SCOPE_CYCLE_COUNTER(
                STAT_CkPathNetwork_CorridorCompile);

            for (auto SpanIndex = 0;
                 SpanIndex < AcceptedSpans.Num();)
            {
                const auto& Span = AcceptedSpans[SpanIndex];

                if (Span._IsOffPath)
                {
                    auto Leg = FCk_PathNetwork_CorridorLeg{};
                    Leg._LegType = ECk_PathNetwork_CorridorLegType::OffPath;
                    Leg._Waypoints = AcceptedOffPathWaypoints.FindChecked(SpanIndex);

                    AppendCompiledPath(Leg.Get_Waypoints(), INDEX_NONE);
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
                    auto StoredRunSpans = TArray<FRouteLegSpan>{};
                    StoredRunSpans.Append(RunSpans.GetData(), RunSpans.Num());
                    const auto RibbonRunIndex = RibbonRuns.Add(MoveTemp(StoredRunSpans));
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
                    auto LastContainmentFailure = FRibbonContainmentFailure{};
                    auto LastContainmentPhase = TEXT("None");
                    int32 LastContainmentAttempt = INDEX_NONE;
                    int32 LastContainmentSegmentIndex = INDEX_NONE;
                    auto LastContainmentSegmentCount = 0;
                    auto LastContainmentFrom = FVector::ZeroVector;
                    auto LastContainmentTo = FVector::ZeroVector;
                    constexpr auto CompileAttempts = 3;
                    for (auto CompileAttempt = 0; CompileAttempt < CompileAttempts; ++CompileAttempt)
                    {
                        auto AttemptParams = CompileParams;
                        if (CompileAttempt >= 1)
                        { AttemptParams._CornerSmoothingDistance = 0.0f; }
                        if (CompileAttempt >= 2)
                        { AttemptParams._SideKeepingFraction = 0.0f; }

                        auto CandidateWaypoints = TArray<FVector>{};
                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonCompile);
                            CandidateWaypoints = Compile_OnRibbonRun(
                                BuiltNetwork,
                                RunSpans,
                                AttemptParams);
                        }
                        if (CandidateWaypoints.Num() < 2)
                        {
                            LastCompileFailure = ERouteFailureStage::RibbonCompileEmpty;
                            continue;
                        }

                        auto Projected = false;
                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonProject);
                            Projected = Try_ProjectPathOntoNavmesh(
                                World,
                                CandidateWaypoints,
                                FilterClass,
                                RibbonProjectionPlanarExtentCm);
                        }
                        if (NOT Projected)
                        {
                            LastCompileFailure = ERouteFailureStage::RibbonProjection;
                            continue;
                        }

                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonProjContain);
                            auto ContainmentFailure = FRibbonContainmentFailure{};
                            int32 ContainmentSegmentIndex = INDEX_NONE;
                            if (NOT Is_PathInsideRibbonRun(
                                    BuiltNetwork,
                                    RunSpans,
                                    CandidateWaypoints,
                                    RibbonContainmentSampleSpacingCm,
                                    RibbonContainmentToleranceCm,
                                    &ContainmentFailure,
                                    &ContainmentSegmentIndex))
                            {
                                LastContainmentFailure = ContainmentFailure;
                                LastContainmentPhase = TEXT("Projected");
                                LastContainmentAttempt = CompileAttempt;
                                LastContainmentSegmentIndex = ContainmentSegmentIndex;
                                LastContainmentSegmentCount = CandidateWaypoints.Num() - 1;
                                if (CandidateWaypoints.IsValidIndex(ContainmentSegmentIndex) &&
                                    CandidateWaypoints.IsValidIndex(ContainmentSegmentIndex + 1))
                                {
                                    LastContainmentFrom = CandidateWaypoints[ContainmentSegmentIndex];
                                    LastContainmentTo = CandidateWaypoints[ContainmentSegmentIndex + 1];
                                }
                                LastCompileFailure = ERouteFailureStage::RibbonContainment;
                                continue;
                            }
                        }

                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonClearance);
                            Apply_NavmeshClearance(
                                World,
                                BuiltNetwork,
                                RunSpans,
                                InParams.Get_DesiredNavmeshClearance(),
                                FilterClass,
                                CandidateWaypoints);
                        }
                        // The projected waypoints already sit within RibbonContainmentToleranceCm,
                        // so the resolved-tolerance headroom is the whole corner-offset budget.
                        auto Resolved = false;
                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonResolve);
                            Resolved = Try_ResolvePathOntoNavmesh(
                                World,
                                CandidateWaypoints,
                                FilterClass,
                                InParams.Get_NavmeshResolvedRibbonTolerance());
                        }
                        if (NOT Resolved)
                        {
                            LastCompileFailure = ERouteFailureStage::RibbonNavValidation;
                            continue;
                        }

                        {
                            SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_RibbonResolveContain);
                            const auto ResolvedRibbonTolerance =
                                RibbonContainmentToleranceCm +
                                InParams.Get_NavmeshResolvedRibbonTolerance();
                            auto ContainmentFailure = FRibbonContainmentFailure{};
                            int32 ContainmentSegmentIndex = INDEX_NONE;
                            if (NOT Is_PathInsideRibbonRun(
                                    BuiltNetwork,
                                    RunSpans,
                                    CandidateWaypoints,
                                    RibbonContainmentSampleSpacingCm,
                                    ResolvedRibbonTolerance,
                                    &ContainmentFailure,
                                    &ContainmentSegmentIndex))
                            {
                                LastContainmentFailure = ContainmentFailure;
                                LastContainmentPhase = TEXT("Resolved");
                                LastContainmentAttempt = CompileAttempt;
                                LastContainmentSegmentIndex = ContainmentSegmentIndex;
                                LastContainmentSegmentCount = CandidateWaypoints.Num() - 1;
                                if (CandidateWaypoints.IsValidIndex(ContainmentSegmentIndex) &&
                                    CandidateWaypoints.IsValidIndex(ContainmentSegmentIndex + 1))
                                {
                                    LastContainmentFrom = CandidateWaypoints[ContainmentSegmentIndex];
                                    LastContainmentTo = CandidateWaypoints[ContainmentSegmentIndex + 1];
                                }
                                LastCompileFailure = ERouteFailureStage::RibbonContainment;
                                continue;
                            }
                        }

                        RunWaypoints = MoveTemp(CandidateWaypoints);
                        break;
                    }

                    if (RunWaypoints.Num() < 2)
                    {
                        if (LastCompileFailure == ERouteFailureStage::RibbonContainment)
                        {
                            ck::pathnetwork::Display(
                                TEXT("[PNDiag] PathNetworkFollower [{}] ribbon containment failed: "
                                     "phase [{}], attempt [{}/{}], waypoint segment [{}/{}] [{}] -> [{}], "
                                     "sample [{}/{}] [{}], closest ribbon [{}], span [{}], edge [{}], "
                                     "distance3D [{}]cm, distance2D [{}]cm, vertical [{}]cm, allowed [{}]cm, "
                                     "excess [{}]cm"),
                                InHandle,
                                LastContainmentPhase,
                                LastContainmentAttempt + 1,
                                CompileAttempts,
                                LastContainmentSegmentIndex,
                                LastContainmentSegmentCount,
                                LastContainmentFrom,
                                LastContainmentTo,
                                LastContainmentFailure._SampleIndex,
                                LastContainmentFailure._SampleCount,
                                LastContainmentFailure._Sample,
                                LastContainmentFailure._ClosestRibbonPoint,
                                LastContainmentFailure._SpanIndex,
                                LastContainmentFailure._EdgeId,
                                LastContainmentFailure._Distance3D,
                                LastContainmentFailure._Distance2D,
                                LastContainmentFailure._VerticalDistance,
                                LastContainmentFailure._AllowedDistance,
                                LastContainmentFailure._Distance3D - LastContainmentFailure._AllowedDistance);
                        }
                        FailureStage = LastCompileFailure;
                        FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                        return;
                    }

                    AppendCompiledPath(RunWaypoints, RibbonRunIndex);
                }
            }
        }

        {
            SCOPE_CYCLE_COUNTER(
                STAT_CkPathNetwork_FinalNavValidate);

            auto NormalizedStart = FVector::ZeroVector;
            auto NormalizedGoal = FVector::ZeroVector;
            {
                SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_FinalEndpoints);
                if (NOT Try_ProjectRouteEndpointOntoNavmesh(
                    World,
                    StartLocation,
                    FilterClass,
                    NormalizedStart))
                {
                    FailureStage = ERouteFailureStage::StartProjection;
                    FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                    return;
                }

                // The agent stands at the first waypoint already — drop it so movement consumers don't
                // backtrack (same artifact CkNavigation's first-waypoint skip addresses).
                if (Result._CompiledWaypoints.Num() > 1 &&
                    FVector::Dist(Result._CompiledWaypoints[0], NormalizedStart) <
                        CompiledWaypointMergeDistance)
                {
                    Result._CompiledWaypoints.RemoveAt(0);
                    if (NOT CompiledSegmentRibbonRunIndices.IsEmpty())
                    { CompiledSegmentRibbonRunIndices.RemoveAt(0); }
                    if (NOT CompiledSegmentNeedsValidation.IsEmpty())
                    { CompiledSegmentNeedsValidation.RemoveAt(0); }
                }

                if (NOT Try_ProjectRouteEndpointOntoNavmesh(
                    World,
                    GoalLocation,
                    FilterClass,
                    NormalizedGoal))
                {
                    FailureStage = ERouteFailureStage::GoalProjection;
                    FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                    return;
                }

                const auto HasExactTerminal =
                    Result._CompiledWaypoints.Num() > 0 &&
                    FVector::Dist(Result._CompiledWaypoints.Last(), NormalizedGoal) <
                        CompiledWaypointMergeDistance;
                if (NOT HasExactTerminal)
                {
                    FailureStage = ERouteFailureStage::TerminalMismatch;
                    FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                    return;
                }
                Result._CompiledWaypoints.Last() = NormalizedGoal;
                if (NOT CompiledSegmentNeedsValidation.IsEmpty())
                { CompiledSegmentNeedsValidation.Last() = true; }
            }

            auto FullMovementPath = Result._CompiledWaypoints;
            FullMovementPath.Insert(NormalizedStart, 0);
            auto FullMovementSegmentRibbonRunIndices =
                CompiledSegmentRibbonRunIndices;
            FullMovementSegmentRibbonRunIndices.Insert(INDEX_NONE, 0);
            auto FullMovementSegmentNeedsValidation =
                CompiledSegmentNeedsValidation;
            FullMovementSegmentNeedsValidation.Insert(true, 0);
            auto FinalContainmentFailure = FRibbonContainmentFailure{};
            int32 FinalContainmentOriginalSegmentIndex = INDEX_NONE;
            int32 FinalContainmentRibbonRunIndex = INDEX_NONE;
            auto FullPathResolution = EConstrainedPathResolution{};
            {
                SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_FinalResolve);
                FullPathResolution =
                    Try_ResolvePathOntoNavmeshWithRibbonConstraints(
                        World,
                        FullMovementPath,
                        FullMovementSegmentRibbonRunIndices,
                        FullMovementSegmentNeedsValidation,
                        BuiltNetwork,
                        RibbonRuns,
                        RibbonContainmentToleranceCm +
                        InParams.Get_NavmeshResolvedRibbonTolerance(),
                        FilterClass,
                        FinalContainmentFailure,
                        FinalContainmentOriginalSegmentIndex,
                        FinalContainmentRibbonRunIndex);
            }

            if (FullPathResolution ==
                EConstrainedPathResolution::RibbonContainmentFailed)
            {
                ck::pathnetwork::Display(
                    TEXT("[PNDiag] PathNetworkFollower [{}] final resolved path left its "
                         "sidewalk ribbon: original segment [{}], ribbon run [{}], "
                         "run span [{}], sample [{}], closest ribbon [{}], edge [{}], "
                         "distance3D [{}]cm, allowed [{}]cm, excess [{}]cm"),
                    InHandle,
                    FinalContainmentOriginalSegmentIndex,
                    FinalContainmentRibbonRunIndex,
                    FinalContainmentFailure._SpanIndex,
                    FinalContainmentFailure._Sample,
                    FinalContainmentFailure._ClosestRibbonPoint,
                    FinalContainmentFailure._EdgeId,
                    FinalContainmentFailure._Distance3D,
                    FinalContainmentFailure._AllowedDistance,
                    FinalContainmentFailure._Distance3D -
                        FinalContainmentFailure._AllowedDistance);
                FailureStage = ERouteFailureStage::RibbonContainment;
                FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                return;
            }
            if (FullPathResolution != EConstrainedPathResolution::Succeeded)
            {
                FailureStage = ERouteFailureStage::FullPathValidation;
                FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                return;
            }

            const auto ResolvedHasExactTerminal =
                FullMovementPath.Num() > 0 &&
                FVector::Dist(FullMovementPath.Last(), NormalizedGoal) <
                    CompiledWaypointMergeDistance;
            if (NOT ResolvedHasExactTerminal)
            {
                FailureStage = ERouteFailureStage::TerminalMismatch;
                FailRoute(ECk_PathNetwork_RouteFailReason::NoRouteFound);
                return;
            }
            FullMovementPath.Last() = NormalizedGoal;

            if (FullMovementPath.Num() > 1 &&
                FVector::Dist(FullMovementPath[0], NormalizedStart) <
                    CompiledWaypointMergeDistance)
            { FullMovementPath.RemoveAt(0); }
            Result._CompiledWaypoints = MoveTemp(FullMovementPath);
        }

        InCorridor._Result = MoveTemp(Result);
        InCorridor._Network = Network;
        InCorridor._NavQueryFilter = InRequest.Get_NavQueryFilter();
        InCorridor._NetworkEpoch = GraphFragment.Get_Epoch();

        auto Follower = InHandle;
        {
            SCOPE_CYCLE_COUNTER(
                STAT_CkPathNetwork_RouteSignal);
            UUtils_Signal_PathNetworkFollower_OnRouteReady::Broadcast(Follower, MakePayload(Follower, InCorridor.Get_Result()));
        }

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
        const auto ResolvedRibbonTolerance =
            Tuning.Get_NavmeshResolvedRibbonTolerance();
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
            && FMath::IsFinite(Clearance) && Clearance >= 0.0f && Clearance <= 1000.0f
            && FMath::IsFinite(ResolvedRibbonTolerance)
            && ResolvedRibbonTolerance >= 0.0f
            && ResolvedRibbonTolerance <= 100.0f;
        CK_ENSURE_IF_NOT(TuningIsValid,
            TEXT("PathNetworkFollower tuning request contains invalid values "
                 "(far/direct multiplier [{}], near multiplier [{}], network gap multiplier [{}], join max [{}], transfer max [{}], local shortcut max [{}], direct grace [{}], minimum direct savings [{}], "
                 "side [{}], spacing [{}], smoothing [{}], clearance [{}], resolved ribbon tolerance [{}])"),
            Multiplier, NearEndpointMultiplier, NetworkGapMultiplier, JoinMaxDistance, TransferMaxDistance,
            LocalShortcutMaxDistance, DirectGraceDistance,
            DirectMinimumSavings,
            SideKeeping, Spacing, Smoothing, Clearance, ResolvedRibbonTolerance)
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
        InParams.Set_NavmeshResolvedRibbonTolerance(ResolvedRibbonTolerance);
        ++InParams._TuningRevision;
        OutResult = ECk_Request_OperationResult::Succeeded;

        const auto Status = InCorridor._Result.Get_Status();
        if (Status != ECk_PathNetwork_RouteStatus::Ready && Status != ECk_PathNetwork_RouteStatus::Pending)
        { return; }

        auto Replan = FCk_Request_PathNetworkFollower_FindRoute{InCorridor._Result.Get_GoalLocation()};
        Replan.Set_Network(InCorridor._Network);
        Replan.Set_NavQueryFilter(InCorridor._NavQueryFilter);
        Replan.Set_TuningRevision(InParams.Get_TuningRevision());
        Replan.Set_RequestRevision(InCorridor._Result.Get_RequestRevision());
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
        Request.Set_NavQueryFilter(InCorridor.Get_NavQueryFilter());
        Request.Set_TuningRevision(InParams.Get_TuningRevision());
        Request.Set_RequestRevision(InCorridor.Get_Result().Get_RequestRevision());

        auto NonConstHandle = InHandle;
        NonConstHandle.AddOrGet<FFragment_PathNetworkFollower_Requests>()._Requests.Emplace(Request);

        InCorridor._Result._Status = ECk_PathNetwork_RouteStatus::Pending;
        InCorridor._NetworkEpoch = CurrentEpoch;

        ck::pathnetwork::Verbose(TEXT("PathNetworkFollower [{}] corridor invalidated by network rebuild (epoch [{}]) — replanning"),
            InHandle, CurrentEpoch);
    }
}

// --------------------------------------------------------------------------------------------------------------------
