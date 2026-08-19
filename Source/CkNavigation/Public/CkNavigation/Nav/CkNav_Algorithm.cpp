#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/CkNavigation_Stats.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Signal/CkSignal_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/NavMeshPath.h>
#include <NavMesh/RecastNavMesh.h>
#include <NavigationData.h>
#include <NavFilters/NavigationQueryFilter.h>
#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Nav::FindPathSync"),     STAT_Nav_FindPathSync,     STATGROUP_CkNav);
DECLARE_CYCLE_STAT(TEXT("Nav::ProjectStartEnd"),  STAT_Nav_ProjectStartEnd,  STATGROUP_CkNav);
DECLARE_CYCLE_STAT(TEXT("Nav::RecastFindPath"),   STAT_Nav_RecastFindPath,   STATGROUP_CkNav);

DECLARE_DWORD_COUNTER_STAT(TEXT("Nav Path Queries"),       STAT_Nav_PathQueries,       STATGROUP_CkNav);
DECLARE_DWORD_COUNTER_STAT(TEXT("Nav Waypoints Extracted"), STAT_Nav_WaypointsExtracted, STATGROUP_CkNav);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    FindPathSync(
        UNavigationSystemV1& InNavSys,
        ARecastNavMesh&      InNavData,
        const FVector&       InStart,
        const FVector&       InEnd,
        bool                 InAllowPartial,
        float                InProjectionHalfExtent,
        float                InProjectionVerticalHalfExtent,
        float                InAgentRadiusForFirstSkip,
        FCk_Nav_PathResult&  OutResult,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        float                InCornerOffsetDistance)
        -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_Nav_FindPathSync);
    INC_DWORD_STAT(STAT_Nav_PathQueries);

    auto& Diag = OutResult._Diagnostics;
    Diag = FCk_Nav_PathDiagnostics{};
    Diag._LastTargetLocation = InEnd;
    Diag._LastAgentLocation  = InStart;
    Diag._LastQueryWallTime  = FPlatformTime::Seconds();

    OutResult._DestinationLocation = InEnd;

    const auto VerticalHalfExtent = (InProjectionVerticalHalfExtent < 0.0f)
        ? InProjectionHalfExtent
        : InProjectionVerticalHalfExtent;
    const auto ProjectionExtent = FVector{InProjectionHalfExtent, InProjectionHalfExtent, VerticalHalfExtent};
    auto StartProj = FNavLocation{};
    auto EndProj   = FNavLocation{};
    auto bStartProjected = false;
    auto bEndProjected   = false;
    {
        SCOPE_CYCLE_COUNTER(STAT_Nav_ProjectStartEnd);
        bStartProjected = InNavSys.ProjectPointToNavigation(InStart, StartProj, ProjectionExtent, &InNavData);
        bEndProjected   = InNavSys.ProjectPointToNavigation(InEnd,   EndProj,   ProjectionExtent, &InNavData);
    }

    Diag._StartProjected     = bStartProjected;
    Diag._EndProjected       = bEndProjected;
    Diag._LastProjectedStart = bStartProjected ? StartProj.Location : FVector::ZeroVector;
    Diag._LastProjectedEnd   = bEndProjected   ? EndProj.Location   : FVector::ZeroVector;

    const auto FinishWithDuration = [&]()
    {
        Diag._LastQueryDurationMs = static_cast<float>((FPlatformTime::Seconds() - Diag._LastQueryWallTime) * 1000.0);
    };

    const auto LogProjectionFailure = [&](const FString& InWhich, const FVector& InPoint)
    {
        const auto NavBounds = InNavData.GetNavMeshBounds();

        const auto BigHalfExtentUu = InProjectionHalfExtent * 8.0f;
        const auto BigExtent = FVector{BigHalfExtentUu};
        auto BigProj = FNavLocation{};
        const auto bBigOk = InNavSys.ProjectPointToNavigation(InPoint, BigProj, BigExtent, &InNavData);

        const auto BigOkStr = FString{bBigOk ? TEXT("OK") : TEXT("FAIL")};

        const auto LegacyCubeApplies = VerticalHalfExtent < InProjectionHalfExtent;
        auto LegacyProj = FNavLocation{};
        const auto LegacyOk = LegacyCubeApplies
            && InNavSys.ProjectPointToNavigation(InPoint, LegacyProj, FVector{InProjectionHalfExtent}, &InNavData);
        const auto LegacyCubeStr = FString{NOT LegacyCubeApplies ? TEXT("n/a") : LegacyOk ? TEXT("OK") : TEXT("FAIL")};

        ck::nav::Warning(TEXT("FindPathSync: [{}] projection FAILED. "
            "Point=[{}] Extent=[{}]. NavBounds=[{} -> {}] (valid=[{}]). "
            "Retry@[{}]uu=[{}] (snapped=[{}]). "
            "LegacyCube@[{}]uu=[{}] (snapped=[{}]). "
            "DefaultFilterValid=[{}] AgentCfg=[r=[{}] h=[{}] step=[{}]]"),
            InWhich,
            InPoint, ProjectionExtent,
            NavBounds.Min, NavBounds.Max, static_cast<int32>(NavBounds.IsValid != 0),
            BigHalfExtentUu,
            BigOkStr,
            bBigOk ? BigProj.Location : FVector::ZeroVector,
            InProjectionHalfExtent,
            LegacyCubeStr,
            LegacyOk ? LegacyProj.Location : FVector::ZeroVector,
            static_cast<int32>(InNavData.GetDefaultQueryFilter().IsValid()),
            InNavData.GetConfig().AgentRadius, InNavData.GetConfig().AgentHeight, InNavData.GetConfig().AgentStepHeight);
    };

    if (NOT bStartProjected)
    {
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::StartProjectFailed;
        LogProjectionFailure(FString{TEXT("Start")}, InStart);
        FinishWithDuration();
        return false;
    }

    if (NOT bEndProjected)
    {
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::EndProjectFailed;
        LogProjectionFailure(FString{TEXT("End")}, InEnd);
        FinishWithDuration();
        return false;
    }

    auto QueryFilter = InNavData.GetDefaultQueryFilter();
    if (InFilterClass.Get() != nullptr)
    { QueryFilter = UNavigationQueryFilter::GetQueryFilter(InNavData, InFilterClass); }

    if (NOT QueryFilter.IsValid())
    {
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::NoDefaultFilter;
        FinishWithDuration();
        return false;
    }

    auto Query = FPathFindingQuery{
        /* Owner */          nullptr,
        /* NavData */        InNavData,
        /* Start */          StartProj.Location,
        /* End */            EndProj.Location,
        /* SourceFilter */   QueryFilter
    };
    Query.SetAllowPartialPaths(InAllowPartial);

    auto Result = FPathFindingResult{};
    {
        SCOPE_CYCLE_COUNTER(STAT_Nav_RecastFindPath);
        Result = ARecastNavMesh::FindPath(Query.NavAgentProperties, Query);
    }

    const auto ShouldOffsetCorners =
        FMath::IsFinite(InCornerOffsetDistance)
        && InCornerOffsetDistance > 0.0f;
    if (ShouldOffsetCorners && Result.Path.IsValid())
    {
        auto* NavMeshPath = Result.Path->CastPath<FNavMeshPath>();
        if (NavMeshPath != nullptr)
        { NavMeshPath->OffsetFromCorners(InCornerOffsetDistance); }
    }

    ExtractWaypoints(Result, InStart, InAgentRadiusForFirstSkip, OutResult);
    FinishWithDuration();

    return OutResult._Status == ECk_Nav_PathStatus::Ready
        || OutResult._Status == ECk_Nav_PathStatus::Partial;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    ExtractWaypoints(
        const FPathFindingResult& InNavResult,
        const FVector&            InAgentLocation,
        float                     InAgentRadius,
        FCk_Nav_PathResult&       OutResult)
        -> void
{
    auto& Diag = OutResult._Diagnostics;
    Diag._RawPathPointCount = InNavResult.Path.IsValid() ? InNavResult.Path->GetPathPoints().Num() : 0;

    const auto bSuccess = InNavResult.Result == ENavigationQueryResult::Success;

    if (NOT bSuccess || NOT InNavResult.Path.IsValid())
    {
        // _Waypoints is deliberately left intact so consumers keep walking the old path.
        OutResult._Status = ECk_Nav_PathStatus::Failed;

        switch (InNavResult.Result)
        {
            case ENavigationQueryResult::Error:   Diag._LastFailReason = ECk_Nav_PathFailReason::FindPathError;   break;
            case ENavigationQueryResult::Fail:    Diag._LastFailReason = ECk_Nav_PathFailReason::FindPathNoPath;  break;
            case ENavigationQueryResult::Invalid: Diag._LastFailReason = ECk_Nav_PathFailReason::FindPathInvalid; break;
            default:                              Diag._LastFailReason = ECk_Nav_PathFailReason::FindPathError;   break;
        }
        Diag._ExtractedWaypointCount = 0;
        return;
    }

    const auto& Points = InNavResult.Path->GetPathPoints();
    auto NewWaypoints = TArray<FVector>{};
    NewWaypoints.Reserve(Points.Num());

    constexpr auto SkipFirstRadiusMultiplier = 2.0f;
    const auto SkipFirstThresholdSquared = (InAgentRadius > 0.0f)
        ? FMath::Square(InAgentRadius * SkipFirstRadiusMultiplier)
        : -1.0f;

    for (auto i = 0; i < Points.Num(); ++i)
    {
        const auto& P = Points[i].Location;

        if (i == 0 && SkipFirstThresholdSquared > 0.0f &&
            FVector::DistSquared(P, InAgentLocation) <= SkipFirstThresholdSquared)
        { continue; }

        NewWaypoints.Emplace(P);
    }

    Diag._ExtractedWaypointCount = NewWaypoints.Num();
    INC_DWORD_STAT_BY(STAT_Nav_WaypointsExtracted, NewWaypoints.Num());

    if (NewWaypoints.IsEmpty())
    {
        // Reported as a failure so listeners never see an empty waypoint list as Ready.
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::EmptyPath;
        return;
    }

    OutResult._Waypoints = MoveTemp(NewWaypoints);
    OutResult._Status    = InNavResult.IsPartial() ? ECk_Nav_PathStatus::Partial : ECk_Nav_PathStatus::Ready;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    InstallExternalPath(
        FCk_Handle&     InHandle,
        TArray<FVector> InWaypoints,
        const FVector&  InDestination,
        int32           InRequestRevision)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("InstallExternalPath called with an invalid Handle"))
    { return; }

    CK_ENSURE_IF_NOT(NOT InWaypoints.IsEmpty(),
        TEXT("InstallExternalPath on [{}] called with an empty waypoint list — refusing to install "
             "(an empty Ready path would stall every consumer that walks it)"), InHandle)
    { return; }

    auto& Result = InHandle.AddOrGet<ck::FFragment_Nav_PathResult>();

    Result._Waypoints           = MoveTemp(InWaypoints);
    Result._DestinationLocation = InDestination;
    Result._Status              = ECk_Nav_PathStatus::Ready;
    // External providers share this result slot with CkNavigation. Retaining
    // the caller's revision makes this install the writer authority: an older
    // deferred Recast request cannot subsequently replace this route.
    Result._RequestRevision     = InRequestRevision;

    // No navmesh query ran, so only the fields an external provider can honestly report are set.
    Result._Diagnostics                          = FCk_Nav_PathDiagnostics{};
    Result._Diagnostics._LastTargetLocation      = InDestination;
    Result._Diagnostics._ExtractedWaypointCount  = Result._Waypoints.Num();

    ck::nav::Verbose(TEXT("External path installed on [{}] ([{}] waypoints, destination {})"),
        InHandle, Result._Waypoints.Num(), InDestination);

    ck::UUtils_Signal_Nav_OnPathReady::Broadcast(
        InHandle, ck::MakePayload(InHandle, Result));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    MarkPathPending(
        FCk_Handle& InHandle,
        int32       InRequestRevision)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("MarkPathPending called with an invalid Handle"))
    { return; }

    auto& Result = InHandle.AddOrGet<ck::FFragment_Nav_PathResult>();
    Result._Status = ECk_Nav_PathStatus::Pending;
    Result._RequestRevision = InRequestRevision;
    Result._PendingSinceSeconds = FPlatformTime::Seconds();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    AbandonPath(
        FCk_Handle& InHandle,
        int32       InRequestRevision)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("AbandonPath called with an invalid Handle"))
    { return; }

    // Nothing was ever acquired on this entity, so there is nothing to release.
    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return; }

    auto& Result = InHandle.Get<ck::FFragment_Nav_PathResult>();
    Result._Status = ECk_Nav_PathStatus::None;
    Result._RequestRevision = InRequestRevision;
    Result._Waypoints.Reset();
    Result._DestinationLocation = FVector::ZeroVector;
    Result._Diagnostics._LastFailReason = ECk_Nav_PathFailReason::None;
    Result._PendingSinceSeconds = 0.0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    FailPath(
        FCk_Handle&            InHandle,
        ECk_Nav_PathFailReason InReason,
        int32                  InRequestRevision)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("FailPath called with an invalid Handle"))
    { return; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return; }

    auto& Result = InHandle.Get<ck::FFragment_Nav_PathResult>();
    Result._Status = ECk_Nav_PathStatus::Failed;
    Result._RequestRevision = InRequestRevision;
    Result._Diagnostics._LastFailReason = InReason;
    Result._PendingSinceSeconds = 0.0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    AgePathPending(
        FCk_Handle& InHandle,
        float       InAgeBySeconds)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("AgePathPending called with an invalid Handle"))
    { return; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return; }

    auto& Result = InHandle.Get<ck::FFragment_Nav_PathResult>();
    if (Result._PendingSinceSeconds <= 0.0)
    { return; }

    Result._PendingSinceSeconds -= static_cast<double>(InAgeBySeconds);
}

// --------------------------------------------------------------------------------------------------------------------
