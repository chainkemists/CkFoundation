#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include <CoreMinimal.h>
#include <Templates/SubclassOf.h>

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class UNavigationQueryFilter;
class UNavigationSystemV1;
struct FPathFindingResult;

// --------------------------------------------------------------------------------------------------------------------
// Static utilities for Recast/UE NavigationSystem path queries. No dtCrowd state, no
// agent abstraction — just synchronous wrappers over ARecastNavMesh::FindPath plus a
// post-processing pass to convert FPathFindingResult into the FCk_Nav_PathResult shape
// the rest of the codebase consumes.
//
// FindPathSync is the primary entrypoint. Callers go through UCk_Utils_Nav_UE
// (request-based, deferred drain). Direct callers — debugger Health Check probe,
// startup smoke tests — use FindPathSync directly.
// --------------------------------------------------------------------------------------------------------------------

struct CKNAVIGATION_API FCk_Nav_Algorithm
{
    // Run a synchronous path query. Populates OutResult with waypoints + status +
    // diagnostics. Returns true on Ready or Partial; false on Failed/Invalid.
    //
    // OutResult._Waypoints is preserved on failure so consumers can keep walking
    // the previous path while deciding what to do.
    static auto FindPathSync(
        UNavigationSystemV1& InNavSys,
        ARecastNavMesh&      InNavData,
        const FVector&       InStart,
        const FVector&       InEnd,
        bool                 InAllowPartial,
        float                InProjectionHalfExtent,         // cm; from project setting (default 500)
        float                InProjectionVerticalHalfExtent, // cm; < 0 => use horizontal extent (uniform cube, today's behavior)
        float                InAgentRadiusForFirstSkip, // cm; 0 disables the skip-first-waypoint pass
        FCk_Nav_PathResult&  OutResult,
        TSubclassOf<UNavigationQueryFilter> InFilterClass = {}) -> bool; // null -> NavData's default filter

    // Convert a raw FPathFindingResult into the FCk_Nav_PathResult shape, updating
    // _Status + _Diagnostics. If InAgentRadius > 0, drops the first waypoint when it
    // is within ~2x radius of InAgentLocation (avoids the "backtrack to start"
    // artifact when UE includes the agent's current position as the first point).
    static auto ExtractWaypoints(
        const FPathFindingResult& InNavResult,
        const FVector&            InAgentLocation,
        float                     InAgentRadius,
        FCk_Nav_PathResult&       OutResult) -> void;

    // The external path-provider seam. Installs an externally-computed polyline (e.g. a
    // CkPathNetwork corridor) onto InHandle's FFragment_Nav_PathResult exactly as if
    // FindPathSync had produced it — status Ready, waypoints + destination populated,
    // Nav_OnPathReady broadcast. Downstream consumers (CkCrowd's OnPathResolved poll,
    // steering) are provider-agnostic by construction.
    static auto InstallExternalPath(
        FCk_Handle&     InHandle,
        TArray<FVector> InWaypoints,
        const FVector&  InDestination) -> void;

    // Companion to InstallExternalPath: parks the entity's path result at Pending while an
    // external provider computes. Without this, pollers (CkCrowd's OnPathResolved) would consume
    // the PREVIOUS Ready result as if it answered the new request — the nav processor performs
    // the equivalent stamp when it starts servicing a FindPath, but no nav request exists when
    // an external provider owns the query. Creates the result fragment if absent.
    static auto MarkPathPending(
        FCk_Handle& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
