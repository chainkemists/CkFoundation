#pragma once

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"
#include "CkNavigation/NavSurface/CkNavFilterDefinition_DataAsset.h"
#include "CkNavigation/NavSurface/CkNavSurface_Fragment_Data.h"

#include <AI/Navigation/NavQueryFilter.h>
#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <Templates/SubclassOf.h>

// --------------------------------------------------------------------------------------------------------------------

class ARecastNavMesh;
class UNavArea;
class UNavigationSystemV1;

// --------------------------------------------------------------------------------------------------------------------
// The Recast provider. Every Unreal-Navigation type in CkNavigation lives behind this surface: the
// area-tag table, the filter compiler, and one entry per neutral capability.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::nav_surface_recast
{
    // Area tags and native filter definitions are contributed by the modules that own the area
    // classes, so the registration cannot run before the gameplay-tag manager exists. A registrar
    // parks the work at static-init time; the table runs every pending one on first use.
    CKNAVIGATION_API auto Register_AreaTag(
        const FGameplayTag& InAreaTag,
        TSubclassOf<UNavArea> InAreaClass) -> void;

    CKNAVIGATION_API auto Register_FilterDefinition(
        const FGameplayTag& InFilterTag,
        FCk_NavFilter_Definition InDefinition) -> void;

    struct CKNAVIGATION_API FRegistrar
    {
        explicit FRegistrar(TFunction<void()> InRegistration);
    };

    CKNAVIGATION_API auto Get_AreaClass(
        const FGameplayTag& InAreaTag) -> TSubclassOf<UNavArea>;

    CKNAVIGATION_API auto Get_RegisteredAreaTags() -> TArray<FGameplayTag>;

    CKNAVIGATION_API auto TryGet_FilterDefinition(
        const FGameplayTag& InFilterTag) -> TOptional<FCk_NavFilter_Definition>;

    // Compiles a filter tag plus a value-only overlay into the engine filter the query runs with.
    // An unmapped tag falls back to the NavData default; a malformed overlay or definition fails
    // closed rather than silently weakening the caller's path policy.
    CKNAVIGATION_API auto Get_CompiledQueryFilter(
        ARecastNavMesh& InNavData,
        const FGameplayTag& InFilterTag,
        const FCk_Nav_QueryFilterOverlay& InOverlay) -> FSharedConstNavQueryFilter;

    // ----------------------------------------------------------------------------------------------------------------

    CKNAVIGATION_API auto TryGet_NavSystem(UWorld* InWorld) -> UNavigationSystemV1*;
    CKNAVIGATION_API auto TryGet_NavData(UWorld* InWorld) -> ARecastNavMesh*;

    CKNAVIGATION_API auto Try_ProjectPoint(
        UWorld* InWorld,
        const FCk_NavSurface_ProjectionQuery& InQuery) -> FCk_NavSurface_ProjectionResult;

    CKNAVIGATION_API auto Try_MoveAlongSurface(
        UWorld* InWorld,
        const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery) -> FCk_NavSurface_MoveAlongSurfaceResult;

    CKNAVIGATION_API auto Try_SurfaceRaycast(
        UWorld* InWorld,
        const FCk_NavSurface_RaycastQuery& InQuery) -> FCk_NavSurface_RaycastResult;

    // Callable off the game thread: stack-local query, no shared mutable state, results written
    // into the returned value.
    CKNAVIGATION_API auto Get_BoundarySegments(
        UWorld* InWorld,
        const FCk_NavSurface_BoundaryQuery& InQuery) -> FCk_NavSurface_BoundaryResult;

    CKNAVIGATION_API auto Get_IsReachable(
        UWorld* InWorld,
        const FCk_NavSurface_ReachabilityQuery& InQuery) -> FCk_NavSurface_ReachabilityResult;

    CKNAVIGATION_API auto Get_SurfaceBounds(UWorld* InWorld) -> FBox;
    CKNAVIGATION_API auto Get_ProviderHealth(UWorld* InWorld) -> ECk_NavSurface_ProviderHealth;
    CKNAVIGATION_API auto Get_IsBuildInProgress(UWorld* InWorld) -> bool;
    CKNAVIGATION_API auto Get_SurfaceRevision(UWorld* InWorld) -> int64;
    CKNAVIGATION_API auto Request_SurfaceRebuild(UWorld* InWorld) -> bool;

    // The "did my paint actually land" probe: the mesh must report the area at that location.
    CKNAVIGATION_API auto Get_IsAreaLiveAt(
        UWorld* InWorld,
        const FGameplayTag& InAreaTag,
        const FVector& InLocation,
        const FVector& InSearchHalfExtents) -> bool;

    // Paints, repaints, or unpaints the markup entity's area. Recast's per-markup state is the
    // nav-octree painter, which it keeps on the markup entity itself.
    CKNAVIGATION_API auto Apply_AreaMarkup(
        UWorld*                                  InWorld,
        FCk_Handle&                              InMarkupEntity,
        const FCk_Request_NavSurface_AreaMarkup& InRequest) -> bool;

    CKNAVIGATION_API auto Get_IsMarkupLive(
        UWorld*           InWorld,
        const FCk_Handle& InMarkupEntity) -> bool;

    // Retires the markup entity's painter and the state that named it.
    CKNAVIGATION_API auto Release_AreaMarkup(
        UWorld*     InWorld,
        FCk_Handle& InMarkupEntity) -> void;
}

// --------------------------------------------------------------------------------------------------------------------
