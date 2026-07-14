#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/CkNavigation_Stats.h"
#include "CkNavigation/Nav/CkNav_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Signal/CkSignal_Utils.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastNavMesh.h>
#include <NavigationData.h>
#include <NavFilters/NavigationQueryFilter.h>

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
        TSubclassOf<UNavigationQueryFilter> InFilterClass)
        -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_Nav_FindPathSync);
    INC_DWORD_STAT(STAT_Nav_PathQueries);

    // Reset and seed per-query diagnostics. Captured for every attempt (success or failure)
    // so the debugger can show a complete picture of the last path query without round-
    // tripping through logs.
    auto& Diag = OutResult._Diagnostics;
    Diag = FCk_Nav_PathDiagnostics{};
    Diag._LastTargetLocation = InEnd;
    Diag._LastAgentLocation  = InStart;
    Diag._LastQueryWallTime  = FPlatformTime::Seconds();

    OutResult._DestinationLocation = InEnd;

    // Project Start/End onto the navmesh before issuing the path query. UE's internal
    // FindPath does its own findNearestPoly using NavData's DefaultQueryExtent (derived
    // from agent radius/height); when the caller's points are far above/below the navmesh
    // surface, that lookup misses and FindPath returns Error with an allocated-but-empty
    // path. Projecting up-front with a generous extent guarantees the path query receives
    // points that lie ON the navmesh surface.
    // Vertical (Z) reach is a separate opt-in knob: a negative InProjectionVerticalHalfExtent
    // reproduces the uniform cube (today's behavior); a smaller value keeps the horizontal
    // reach but stops Start/End from snapping onto a stray navmesh surface far above/below the
    // intended floor (the checkout sink/float root cause).
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

    // When projection fails, log a diagnostic snapshot: nav bounds, the failing point,
    // the extent we used, and a retry with a much larger extent. If the retry succeeds we
    // know the configured extent is somehow too small (Recast<->UE axis quirk, agent params
    // skewing the search box, etc). If it ALSO fails, projection itself is broken (NavData
    // impl-swap, navmesh tiles in a transient state, etc).
    const auto LogProjectionFailure = [&](const FString& InWhich, const FVector& InPoint)
    {
        const auto NavBounds = InNavData.GetNavMeshBounds();

        const auto BigHalfExtentUu = InProjectionHalfExtent * 8.0f;
        const auto BigExtent = FVector{BigHalfExtentUu};
        auto BigProj = FNavLocation{};
        const auto bBigOk = InNavSys.ProjectPointToNavigation(InPoint, BigProj, BigExtent, &InNavData);

        const auto BigOkStr = FString{bBigOk ? TEXT("OK") : TEXT("FAIL")};

        // Legacy-cube probe: when the vertical clamp is active, ALSO test the pre-clamp
        // uniform cube so the log line itself answers "would the old behavior have snapped
        // this point?" — the decisive datum when triaging whether a NO PATH report is a
        // clamp regression or a pre-existing content/nav defect.
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

    // The parameterized FPathFindingQuery ctor populates NavAgentProperties from
    // InNavData.GetConfig() and derives the DefaultFilter from NavData when SourceFilter
    // is nullptr. Passing it explicitly avoids relying on that fallback.
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

    // ARecastNavMesh::FindPath skips the NavSys agent-dispatch step. The
    // Query.NavAgentProperties was populated from InNavData.GetConfig() by the ctor.
    auto Result = FPathFindingResult{};
    {
        SCOPE_CYCLE_COUNTER(STAT_Nav_RecastFindPath);
        Result = ARecastNavMesh::FindPath(Query.NavAgentProperties, Query);
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
        // PRESERVE _Waypoints on failure so consumers can keep walking the old path while
        // deciding what to do. Only update status + diagnostics.
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

    // Skip-first-waypoint pass: drop the leading point if it is within ~2x agent radius
    // of the agent's actual location. Avoids the "backtrack to start" artifact when UE
    // includes the agent's current position as the path's starting waypoint. Disabled
    // when InAgentRadius <= 0.
    const auto SkipFirstThresholdSquared = (InAgentRadius > 0.0f)
        ? FMath::Square(InAgentRadius * 2.0f)
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
        // Result=Success but post-extract path has zero waypoints — degenerate (start≈end
        // after the skip-first pass, or UE returned a 1-point path). Report as failure so
        // listeners don't treat an empty waypoint list as Ready.
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
        const FVector&  InDestination)
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

    // Keep the debugger's picture coherent: this result did not come from a navmesh query, so
    // only the fields an external provider can honestly report are populated.
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
        FCk_Handle& InHandle)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("MarkPathPending called with an invalid Handle"))
    { return; }

    InHandle.AddOrGet<ck::FFragment_Nav_PathResult>()._Status = ECk_Nav_PathStatus::Pending;
}

// --------------------------------------------------------------------------------------------------------------------
