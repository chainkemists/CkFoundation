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
// Synchronous wrappers over ARecastNavMesh::FindPath. Game code goes through UCk_Utils_Nav_UE
// (request-based, deferred drain); direct FindPathSync calls are diagnostics-only.
// --------------------------------------------------------------------------------------------------------------------

struct CKNAVIGATION_API FCk_Nav_Algorithm
{
    // True on Ready/Partial, false on Failed/Invalid. OutResult._Waypoints is preserved on
    // failure so consumers can keep walking the previous path.
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

    // InAgentRadius > 0 drops the first waypoint when it is within ~2x radius of InAgentLocation:
    // UE includes the agent's own position as the first point, which reads as a backtrack-to-start.
    static auto ExtractWaypoints(
        const FPathFindingResult& InNavResult,
        const FVector&            InAgentLocation,
        float                     InAgentRadius,
        FCk_Nav_PathResult&       OutResult) -> void;

    // The external path-provider seam: installs an externally-computed polyline (e.g. a
    // CkPathNetwork corridor) exactly as if FindPathSync had produced it.
    static auto InstallExternalPath(
        FCk_Handle&     InHandle,
        TArray<FVector> InWaypoints,
        const FVector&  InDestination) -> void;

    // Parks the result at Pending while an external provider computes, so pollers don't consume
    // the PREVIOUS Ready result as the answer to the new request. Creates the fragment if absent.
    static auto MarkPathPending(
        FCk_Handle& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
