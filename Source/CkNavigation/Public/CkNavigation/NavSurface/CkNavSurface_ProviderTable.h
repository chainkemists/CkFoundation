#pragma once

#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <CoreMinimal.h>
#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------
// One provider's answer to every navigation-surface capability, as a table of callables.
//
// The facade dispatches through this rather than calling a provider directly, so which provider
// answers is a per-world value rather than a compile-time edge: the neutral signatures are the whole
// contract, and a provider that cannot fill every one of them is not a provider.
// --------------------------------------------------------------------------------------------------------------------

struct CKNAVIGATION_API FCk_NavSurface_ProviderTable
{
public:
    TFunction<FCk_NavSurface_ProjectionResult(UWorld*, const FCk_NavSurface_ProjectionQuery&)>
        _ProjectPoint;

    TFunction<FCk_NavSurface_MoveAlongSurfaceResult(UWorld*, const FCk_NavSurface_MoveAlongSurfaceQuery&)>
        _MoveAlongSurface;

    TFunction<FCk_NavSurface_RaycastResult(UWorld*, const FCk_NavSurface_RaycastQuery&)>
        _SurfaceRaycast;

    // THREAD CONTRACT: every provider's entry here must be callable off the game thread against an
    // immutable snapshot, because the crowd's parallel avoidance sampler calls it from a worker.
    TFunction<FCk_NavSurface_BoundaryResult(UWorld*, const FCk_NavSurface_BoundaryQuery&)>
        _BoundarySegments;

    TFunction<FCk_NavSurface_ReachabilityResult(UWorld*, const FCk_NavSurface_ReachabilityQuery&)>
        _IsReachable;

    TFunction<FBox(UWorld*)>
        _SurfaceBounds;

    TFunction<ECk_NavSurface_ProviderHealth(UWorld*)>
        _ProviderHealth;

    TFunction<bool(UWorld*)>
        _IsBuildInProgress;

    TFunction<int64(UWorld*)>
        _SurfaceRevision;

    TFunction<bool(UWorld*)>
        _RequestSurfaceRebuild;

public:
    auto Get_IsComplete() const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface
{
    /**
     * Publishes a provider's capability table. An incomplete table is REFUSED rather than stored: a
     * half-registered provider would answer some capabilities and null-call the rest.
     *
     * A second registration for the same provider replaces the first and says so at Display.
     */
    CKNAVIGATION_API auto
    Register_Provider(
        ECk_NavSurface_Provider          InProvider,
        FCk_NavSurface_ProviderTable     InTable) -> void;

    /**
     * The table a provider registered, or nullptr when nothing has registered for it.
     *
     * A POINTER INTO THE REGISTRY, deliberately: registration runs from module startup, before any
     * world exists and therefore before any query can reach here, so the map is never mutated while
     * anybody holds one of these. A provider registered at runtime would break that assumption — and
     * with it every pointer already handed out — which is why registration has no runtime entry point.
     */
    CKNAVIGATION_API auto
    TryGet_ProviderTable(
        ECk_NavSurface_Provider InProvider) -> const FCk_NavSurface_ProviderTable*;

    /**
     * The world's own choice, or the project default when it has not made one.
     *
     * Read from a per-world mirror under a read lock, never from the world's registry: a boundary
     * query is callable off the game thread, and the registry is not. The lock guards the handoff
     * of one enum and nothing else — the query that follows runs without it.
     */
    CKNAVIGATION_API auto
    Get_ProviderForWorld(
        UWorld* InWorld) -> ECk_NavSurface_Provider;

    /**
     * Records a world's choice in the mirror. GAME THREAD ONLY, and called by whoever writes the
     * world entity's provider fragment, so the two never disagree.
     */
    CKNAVIGATION_API auto
    Set_ProviderForWorld(
        UWorld*                 InWorld,
        ECk_NavSurface_Provider InProvider) -> void;

    CKNAVIGATION_API auto
    Get_DefaultProvider() -> ECk_NavSurface_Provider;
}

// --------------------------------------------------------------------------------------------------------------------
