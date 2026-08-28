#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"

#include <AI/Navigation/NavQueryFilter.h>
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
    // Resolves a host-selected base filter, then applies value-only exclusions to a private copy.
    // A non-empty malformed overlay fails closed rather than silently weakening the caller's path policy.
    static auto ResolveQueryFilter(
        ARecastNavMesh& InNavData,
        TSubclassOf<UNavigationQueryFilter> InFilterClass,
        const FCk_Nav_QueryFilterOverlay& InOverlay) -> FSharedConstNavQueryFilter;

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
        TSubclassOf<UNavigationQueryFilter> InFilterClass = {}, // null -> NavData's default filter
        float InCornerOffsetDistance = 0.0f, // cm; 0 preserves the raw Recast corridor corners
        const FCk_Nav_QueryFilterOverlay& InQueryFilterOverlay = {}) -> bool;

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
        const FVector&  InDestination,
        int32           InRequestRevision = 0) -> void;

    // Prepends a caller-owned physical approach/escape prefix without changing the provider's
    // status, destination, diagnostics, or revision. Returns the retained prefix count.
    static auto PrependWaypoints(
        FCk_Nav_PathResult& InOutResult,
        TArray<FVector> InLeadingWaypoints) -> int32;

    // Parks the result at Pending while an external provider computes, so pollers don't consume
    // the PREVIOUS Ready result as the answer to the new request. Creates the fragment if absent.
    static auto MarkPathPending(
        FCk_Handle& InHandle,
        int32       InRequestRevision = 0) -> void;

    // Releases what MarkPathPending / a FindPath acquired: the slot returns to None and carries
    // the caller's post-abandon revision, so a query that drains afterwards is recognised as
    // superseded rather than applied. Clears the corridor too — a None slot still holding the old
    // waypoints and destination is a half-cleared mirror that later readers would trust.
    static auto AbandonPath(
        FCk_Handle& InHandle,
        int32       InRequestRevision = 0) -> void;

    // Terminates a slot with an explicit failure reason, carrying the caller's CURRENT revision so
    // the consumer that owns the episode still recognises it as its own answer rather than a
    // superseded one. Used by the pending watchdog to convert a never-answered episode into a
    // reportable failure instead of a silent wedge.
    static auto FailPath(
        FCk_Handle&            InHandle,
        ECk_Nav_PathFailReason InReason,
        int32                  InRequestRevision) -> void;

    // Backdates a parked slot's age. Test-only seam — see the ForTesting hooks on UCk_Utils_Nav_UE.
    static auto AgePathPending(
        FCk_Handle& InHandle,
        float       InAgeBySeconds) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
