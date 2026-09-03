#include "CkNavigation_Module.h"

#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#define LOCTEXT_NAMESPACE "FCkNavigationModule"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_navigation_module
{
    auto Make_RecastProviderTable() -> FCk_NavSurface_ProviderTable
    {
        auto Table = FCk_NavSurface_ProviderTable{};

        Table._ProjectPoint = [](UWorld* InWorld, const FCk_NavSurface_ProjectionQuery& InQuery)
        { return ck::nav_surface_recast::Try_ProjectPoint(InWorld, InQuery); };

        Table._MoveAlongSurface = [](UWorld* InWorld, const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery)
        { return ck::nav_surface_recast::Try_MoveAlongSurface(InWorld, InQuery); };

        Table._SurfaceRaycast = [](UWorld* InWorld, const FCk_NavSurface_RaycastQuery& InQuery)
        { return ck::nav_surface_recast::Try_SurfaceRaycast(InWorld, InQuery); };

        Table._BoundarySegments = [](UWorld* InWorld, const FCk_NavSurface_BoundaryQuery& InQuery)
        { return ck::nav_surface_recast::Get_BoundarySegments(InWorld, InQuery); };

        Table._IsReachable = [](UWorld* InWorld, const FCk_NavSurface_ReachabilityQuery& InQuery)
        { return ck::nav_surface_recast::Get_IsReachable(InWorld, InQuery); };

        Table._SurfaceBounds = [](UWorld* InWorld)
        { return ck::nav_surface_recast::Get_SurfaceBounds(InWorld); };

        Table._ProviderHealth = [](UWorld* InWorld)
        { return ck::nav_surface_recast::Get_ProviderHealth(InWorld); };

        Table._IsBuildInProgress = [](UWorld* InWorld)
        { return ck::nav_surface_recast::Get_IsBuildInProgress(InWorld); };

        Table._SurfaceRevision = [](UWorld* InWorld)
        { return ck::nav_surface_recast::Get_SurfaceRevision(InWorld); };

        Table._RequestSurfaceRebuild = [](UWorld* InWorld)
        { return ck::nav_surface_recast::Request_SurfaceRebuild(InWorld); };

        return Table;
    }
}

// --------------------------------------------------------------------------------------------------------------------

void FCkNavigationModule::StartupModule()
{
    // At module startup rather than lazily: the provider registry is read without a lock, and that is
    // only sound while every registration has landed before the first world exists.
    ck::nav_surface::Register_Provider(
        ECk_NavSurface_Provider::Recast, ck_navigation_module::Make_RecastProviderTable());
}

void FCkNavigationModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkNavigationModule, CkNavigation)
