#pragma once

#include "CkNav_Fragment_Data.h"

#include "Detour/DetourLargeWorldCoordinates.h"   // dtReal

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class UNavigationSystemV1;
class dtCrowd;
class dtNavMeshQuery;
struct dtCrowdAgentParams;
struct FPathFindingResult;

using dtPolyRef = uint64;

// --------------------------------------------------------------------------------------------------------------------

struct CKNAVIGATION_API FCk_Nav_Algorithm
{
    // ----------------------------------------------------------------------------------------------------------------
    // Coordinate conversion
    // ----------------------------------------------------------------------------------------------------------------

    // Convert a UE-space FVector to a Recast-space dtReal[3].
    // (dtReal is double when UE's Detour LWC is enabled — the default in this build.)
    static auto ToRecastFloat3(const FVector& InV, dtReal OutR[3]) -> void;

    // Convert a Recast-space dtReal[3] back to a UE-space FVector.
    static auto FromRecastFloat3(const dtReal InR[3]) -> FVector;

    // ----------------------------------------------------------------------------------------------------------------
    // dtCrowd setup
    // ----------------------------------------------------------------------------------------------------------------

    // Build a dtCrowdAgentParams from FCk_Nav_AgentParams. Pure.
    // VERIFIED A4: sets updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION
    // (DetourCrowd.h:207-211, bit values 1|2|4) and obstacleAvoidanceType from _AvoidanceQuality.
    // Without these, _SeparationWeight and _AvoidanceQuality are silent no-ops.
    static auto BuildCrowdAgentParams(const FCk_Nav_AgentParams& InParams) -> dtCrowdAgentParams;

    // Attempt to register agent with dtCrowd. Returns -1 on failure (crowd full).
    // Caller is responsible for tag/fragment bookkeeping on -1.
    static auto RegisterAgent(
        dtCrowd& InCrowd,
        const FVector& InUeSpaceLocation,
        const dtCrowdAgentParams& InCrowdParams) -> int32;

    // ----------------------------------------------------------------------------------------------------------------
    // Navmesh queries
    // ----------------------------------------------------------------------------------------------------------------

    // Query navmesh poly at a world position. Returns {0, ZeroVector} on miss.
    static auto FindNearestPoly(
        const dtNavMeshQuery& InNavQuery,
        const FVector& InUeSpaceLocation,
        float InSearchHalfExtentUu) -> TPair<dtPolyRef, FVector>;

    // ----------------------------------------------------------------------------------------------------------------
    // Path queries
    // ----------------------------------------------------------------------------------------------------------------

    // Run a synchronous path query. Populates OutResult with waypoints + status.
    // Returns true on Ready or Partial; false on Failed/Invalid.
    // Preserves OutResult._Waypoints on failure (consumer may want to continue the old path).
    static auto FindPathSync(
        UNavigationSystemV1& InNavSys,
        ARecastNavMesh& InNavData,
        const FVector& InStart,
        const FVector& InEnd,
        const FCk_Nav_AgentParams& InAgentParams,
        bool InAllowPartial,
        FCk_Nav_PathResult& OutResult) -> bool;

    // Extract waypoints from a FPathFindingResult.Path, dropping the first if it is within
    // ~2x radius of AgentLocation (avoids the "backtrack to start" artifact).
    // Sets OutResult._Status appropriately. Null-checks Result.Path.IsValid() (verified A3).
    static auto ExtractWaypoints(
        const FPathFindingResult& InNavResult,
        const FVector& InAgentLocation,
        float InAgentRadius,
        FCk_Nav_PathResult& OutResult) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
