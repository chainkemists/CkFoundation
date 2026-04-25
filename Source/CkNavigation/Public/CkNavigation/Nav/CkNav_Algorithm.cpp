#include "CkNav_Algorithm.h"

#include "CkNavigation/CkNavigation_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include <NavigationSystem.h>
#include <NavMesh/RecastHelpers.h>
#include <NavMesh/RecastNavMesh.h>
#include <NavigationData.h>

#include "DetourCrowd/DetourCrowd.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Nav_Algorithm::
    ToRecastFloat3(
        const FVector& InV,
        float OutR[3])
        -> void
{
    // VERIFIED A5: Unreal2RecastPoint at NavMesh/RecastHelpers.h:25-36.
    const auto R = Unreal2RecastPoint(InV);
    OutR[0] = static_cast<float>(R.X);
    OutR[1] = static_cast<float>(R.Y);
    OutR[2] = static_cast<float>(R.Z);
}

auto
    FCk_Nav_Algorithm::
    FromRecastFloat3(
        const float InR[3])
        -> FVector
{
    // VERIFIED A5: Recast2UnrealPoint at NavMesh/RecastHelpers.h:25-36.
    return Recast2UnrealPoint(FVector{InR[0], InR[1], InR[2]});
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

    P.queryFilterType = 0;
    P.userData        = nullptr;

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
    float RecastPos[3];
    ToRecastFloat3(InUeSpaceLocation, RecastPos);

    return InCrowd.addAgent(RecastPos, &InCrowdParams);
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
    float RecastPos[3];
    ToRecastFloat3(InUeSpaceLocation, RecastPos);

    const float HalfExtents[3] = { InSearchHalfExtentUu, InSearchHalfExtentUu, InSearchHalfExtentUu };
    dtQueryFilter Filter{};

    dtPolyRef PolyRef = 0;
    float NearestPt[3] = { 0.0f, 0.0f, 0.0f };
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
    auto Query = FPathFindingQuery{};
    Query.NavData                        = &InNavData;
    Query.StartLocation                  = InStart;
    Query.EndLocation                    = InEnd;
    Query.NavAgentProperties.AgentRadius = InAgentParams.Get_Radius();
    Query.NavAgentProperties.AgentHeight = InAgentParams.Get_Height();
    Query.SetAllowPartialPaths(InAllowPartial);

    OutResult._DestinationLocation = InEnd;

    // VERIFIED A3: NavigationSystem/Public/NavigationData.h:63 hosts FPathFindingResult.
    const auto Result = InNavSys.FindPathSync(Query);

    ExtractWaypoints(Result, InStart, InAgentParams.Get_Radius(), OutResult);

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
    // Verified A3: Result enum lives in ENavigationQueryResult::Type (NavigationTypes.h:626) —
    //   { Invalid, Error, Fail, Success }.
    // Result.Path is FNavPathSharedPtr; null-check via .IsValid() before deref.
    const auto bSuccess = InNavResult.Result == ENavigationQueryResult::Success;

    if (NOT bSuccess || NOT InNavResult.Path.IsValid())
    {
        // Pass-3 / Pass-2 retained: PRESERVE _Waypoints on failure so consumers can keep
        // walking the old path while deciding what to do. Only update status.
        OutResult._Status = ECk_Nav_PathStatus::Failed;
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

    OutResult._Waypoints = MoveTemp(NewWaypoints);
    OutResult._Status    = InNavResult.IsPartial() ? ECk_Nav_PathStatus::Partial : ECk_Nav_PathStatus::Ready;
}

// --------------------------------------------------------------------------------------------------------------------
