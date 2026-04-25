#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastHelpers.h>
#include <NavMesh/RecastNavMesh.h>
#include <NavigationData.h>
#include <NavFilters/NavigationQueryFilter.h>

#include "DetourCrowd/DetourCrowd.h"
#include "Detour/DetourLargeWorldCoordinates.h"   // dtReal

// --------------------------------------------------------------------------------------------------------------------
// NOTE: Detour APIs use `dtReal` (typedef double when DT_LARGE_WORLD_COORDINATES_DISABLED == 0,
// which is the default in this UE build). Methods named ToRecastFloat3 / FromRecastFloat3
// for historical reasons but the array element type is dtReal, not float.
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    ToRecastFloat3(
        const FVector& InV,
        dtReal OutR[3])
        -> void
{
    // VERIFIED A5: Unreal2RecastPoint at NavMesh/RecastHelpers.h:25-36.
    const auto R = Unreal2RecastPoint(InV);
    OutR[0] = static_cast<dtReal>(R.X);
    OutR[1] = static_cast<dtReal>(R.Y);
    OutR[2] = static_cast<dtReal>(R.Z);
}

auto
    FCk_Nav_Algorithm::
    FromRecastFloat3(
        const dtReal InR[3])
        -> FVector
{
    // VERIFIED A5: Recast2UnrealPoint at NavMesh/RecastHelpers.h:25-36.
    return Recast2UnrealPoint(FVector{static_cast<FVector::FReal>(InR[0]),
                                       static_cast<FVector::FReal>(InR[1]),
                                       static_cast<FVector::FReal>(InR[2])});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    BuildCrowdAgentParams(
        const FCk_Nav_AgentParams& InParams)
        -> dtCrowdAgentParams
{
    // VERIFIED A6: dtCrowdAgentParams accepts UE centimeters directly (no Recast scaling).
    dtCrowdAgentParams P{};
    P.radius                = InParams.Get_Radius();
    P.height                = InParams.Get_Height();
    P.maxAcceleration       = InParams.Get_MaxAcceleration();
    P.maxSpeed              = InParams.Get_MaxSpeed();
    P.collisionQueryRange   = InParams.Get_Radius() * 8.0f;
    P.pathOptimizationRange = InParams.Get_Radius() * 30.0f;
    P.separationWeight      = InParams.Get_SeparationWeight();

    // VERIFIED A4 — DT_CROWD_* bits (DetourCrowd.h:207-211):
    //   DT_CROWD_ANTICIPATE_TURNS    = 1 << 0  (1)
    //   DT_CROWD_OBSTACLE_AVOIDANCE  = 1 << 1  (2)
    //   DT_CROWD_SEPARATION          = 1 << 2  (4)
    // Without these flags the SeparationWeight and AvoidanceQuality fields silently do nothing.
    P.updateFlags = static_cast<unsigned char>(
        DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION);

    P.obstacleAvoidanceType = static_cast<unsigned char>(InParams.Get_AvoidanceQuality());

    // dtCrowdAgentParams has `filter` (uchar) for the dtCrowd nav-filter index, default 0.
    // userData / linkFilter are zero-initialized by the {} above.
    P.filter = 0;

    return P;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    RegisterAgent(
        dtCrowd& InCrowd,
        const FVector& InUeSpaceLocation,
        const dtCrowdAgentParams& InCrowdParams)
        -> int32
{
    dtReal RecastPos[3];
    ToRecastFloat3(InUeSpaceLocation, RecastPos);

    // dtCrowd::addAgent(const dtReal* pos, const dtCrowdAgentParams& params, const dtQueryFilter* filter)
    // — params is a reference (not pointer), filter is required (nullptr = default filter).
    return InCrowd.addAgent(RecastPos, InCrowdParams, nullptr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    FindNearestPoly(
        const dtNavMeshQuery& InNavQuery,
        const FVector& InUeSpaceLocation,
        float InSearchHalfExtentUu)
        -> TPair<dtPolyRef, FVector>
{
    dtReal RecastPos[3];
    ToRecastFloat3(InUeSpaceLocation, RecastPos);

    const dtReal HalfExtents[3] = {
        static_cast<dtReal>(InSearchHalfExtentUu),
        static_cast<dtReal>(InSearchHalfExtentUu),
        static_cast<dtReal>(InSearchHalfExtentUu) };
    dtQueryFilter Filter{};

    dtPolyRef PolyRef = 0;
    dtReal NearestPt[3] = { 0.0, 0.0, 0.0 };
    InNavQuery.findNearestPoly(RecastPos, HalfExtents, &Filter, &PolyRef, NearestPt);

    return TPair<dtPolyRef, FVector>{ PolyRef, FromRecastFloat3(NearestPt) };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    FindPathSync(
        UNavigationSystemV1& InNavSys,
        ARecastNavMesh& InNavData,
        const FVector& InStart,
        const FVector& InEnd,
        const FCk_Nav_AgentParams& InAgentParams,
        bool InAllowPartial,
        FCk_Nav_PathResult& OutResult)
        -> bool
{
    // Reset and seed per-query diagnostics. Captured for every attempt (success or failure)
    // so the debugger can show a complete picture of the last path query without round-
    // tripping through logs.
    auto& Diag = OutResult._Diagnostics;
    Diag = FCk_Nav_PathDiagnostics{};
    Diag._LastTargetLocation = InEnd;
    Diag._LastAgentLocation  = InStart;
    Diag._LastQueryWallTime  = FPlatformTime::Seconds();

    OutResult._DestinationLocation = InEnd;

    // Project Start/End onto the navmesh before issuing the path query. UE's internal FindPath
    // does its own findNearestPoly using NavData's DefaultQueryExtent (derived from agent
    // radius/height); when the caller's points are far above/below the navmesh surface, that
    // lookup misses and FindPath returns Error with an allocated-but-empty path. Projecting
    // up-front with a generous extent guarantees the path query receives points that lie ON
    // the navmesh surface.
    constexpr auto ProjectionHalfExtentUu = 500.0f;
    const auto ProjectionExtent = FVector{ProjectionHalfExtentUu};
    auto StartProj = FNavLocation{};
    auto EndProj   = FNavLocation{};
    const auto bStartProjected = InNavSys.ProjectPointToNavigation(InStart, StartProj, ProjectionExtent, &InNavData);
    const auto bEndProjected   = InNavSys.ProjectPointToNavigation(InEnd,   EndProj,   ProjectionExtent, &InNavData);

    Diag._StartProjected     = bStartProjected;
    Diag._EndProjected       = bEndProjected;
    Diag._LastProjectedStart = bStartProjected ? StartProj.Location : FVector::ZeroVector;
    Diag._LastProjectedEnd   = bEndProjected   ? EndProj.Location   : FVector::ZeroVector;

    const auto FinishWithDuration = [&]()
    {
        Diag._LastQueryDurationMs = static_cast<float>((FPlatformTime::Seconds() - Diag._LastQueryWallTime) * 1000.0);
    };

    if (NOT bStartProjected)
    {
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::StartProjectFailed;
        FinishWithDuration();
        return false;
    }

    if (NOT bEndProjected)
    {
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::EndProjectFailed;
        FinishWithDuration();
        return false;
    }

    // The parameterized FPathFindingQuery ctor populates NavAgentProperties from
    // InNavData.GetConfig() and derives the DefaultFilter from NavData when SourceFilter is
    // nullptr. Passing it explicitly avoids relying on that fallback.
    const auto DefaultFilter = InNavData.GetDefaultQueryFilter();

    auto Query = FPathFindingQuery{
        /* Owner */          nullptr,
        /* NavData */        InNavData,
        /* Start */          StartProj.Location,
        /* End */            EndProj.Location,
        /* SourceFilter */   DefaultFilter
    };
    Query.SetAllowPartialPaths(InAllowPartial);

    // Direct ARecastNavMesh::FindPath (RecastNavMesh.h:1375) skips the NavSys agent-dispatch
    // step. Query.NavAgentProperties was populated from InNavData.GetConfig() by the ctor.
    const auto Result = ARecastNavMesh::FindPath(Query.NavAgentProperties, Query);

    ExtractWaypoints(Result, InStart, InAgentParams.Get_Radius(), OutResult);
    FinishWithDuration();

    return OutResult._Status == ECk_Nav_PathStatus::Ready
        || OutResult._Status == ECk_Nav_PathStatus::Partial;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    ExtractWaypoints(
        const FPathFindingResult& InNavResult,
        const FVector& InAgentLocation,
        float InAgentRadius,
        FCk_Nav_PathResult& OutResult)
        -> void
{
    // VERIFIED A3: ENavigationQueryResult::Type at NavigationTypes.h:626 — { Invalid, Error,
    // Fail, Success }. Result.Path is FNavPathSharedPtr; null-check via .IsValid().
    auto& Diag = OutResult._Diagnostics;
    Diag._RawPathPointCount = InNavResult.Path.IsValid() ? InNavResult.Path->GetPathPoints().Num() : 0;

    const auto bSuccess = InNavResult.Result == ENavigationQueryResult::Success;

    if (NOT bSuccess || NOT InNavResult.Path.IsValid())
    {
        // Pass-3 / Pass-2 retained: PRESERVE _Waypoints on failure so consumers can keep
        // walking the old path while deciding what to do. Only update status.
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

    const auto SkipFirstThresholdSquared = FMath::Square(InAgentRadius * 2.0f);

    for (auto i = 0; i < Points.Num(); ++i)
    {
        const auto& P = Points[i].Location;

        // Drop the first point if it is within ~2x radius of agent — avoids "backtrack to start"
        // artifact when UE includes the agent's current position as the path's starting waypoint.
        if (i == 0 && FVector::DistSquared(P, InAgentLocation) <= SkipFirstThresholdSquared)
        { continue; }

        NewWaypoints.Emplace(P);
    }

    Diag._ExtractedWaypointCount = NewWaypoints.Num();

    if (NewWaypoints.IsEmpty())
    {
        // Result=Success but post-extract path has zero waypoints — degenerate (start≈end after
        // the skip-first-if-close pass, or UE returned a 1-point path). Report as failure so
        // listeners don't treat an empty waypoint list as Ready.
        OutResult._Status    = ECk_Nav_PathStatus::Failed;
        Diag._LastFailReason = ECk_Nav_PathFailReason::EmptyPath;
        return;
    }

    OutResult._Waypoints = MoveTemp(NewWaypoints);
    OutResult._Status    = InNavResult.IsPartial() ? ECk_Nav_PathStatus::Partial : ECk_Nav_PathStatus::Ready;
}

// --------------------------------------------------------------------------------------------------------------------
