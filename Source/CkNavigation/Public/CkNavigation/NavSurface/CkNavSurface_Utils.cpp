#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/NavSurface/CkNavSurface_GameplayTags.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_utils
{
    auto Get_World(const UObject* InWorldContext) -> UWorld*
    {
        return ck::IsValid(InWorldContext) ? InWorldContext->GetWorld() : nullptr;
    }

    auto Get_WorldEntity(const UObject* InWorldContext) -> FCk_Handle
    {
        auto* World = Get_World(InWorldContext);
        if (ck::Is_NOT_Valid(World))
        { return {}; }

        return UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_NavSurfaceMarkup_Current>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_ProjectPoint(
        const UObject* InWorldContext,
        const FCk_NavSurface_ProjectionQuery& InQuery)
    -> FCk_NavSurface_ProjectionResult
{
    return ck::nav_surface_recast::Try_ProjectPoint(
        ck_nav_surface_utils::Get_World(InWorldContext), InQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_MoveAlongSurface(
        const UObject* InWorldContext,
        const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery)
    -> FCk_NavSurface_MoveAlongSurfaceResult
{
    return ck::nav_surface_recast::Try_MoveAlongSurface(
        ck_nav_surface_utils::Get_World(InWorldContext), InQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_SurfaceRaycast(
        const UObject* InWorldContext,
        const FCk_NavSurface_RaycastQuery& InQuery)
    -> FCk_NavSurface_RaycastResult
{
    return ck::nav_surface_recast::Try_SurfaceRaycast(
        ck_nav_surface_utils::Get_World(InWorldContext), InQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_BoundarySegments(
        const UObject* InWorldContext,
        const FCk_NavSurface_BoundaryQuery& InQuery,
        TArray<FCk_NavSurface_BoundarySegment>& OutSegments)
    -> ECk_NavSurface_QueryStatus
{
    const auto Result = ck::nav_surface_recast::Get_BoundarySegments(
        ck_nav_surface_utils::Get_World(InWorldContext), InQuery);

    OutSegments = Result.Get_Segments();
    return Result.Get_Status();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_IsReachable(
        const UObject* InWorldContext,
        const FCk_NavSurface_ReachabilityQuery& InQuery)
    -> ECk_NavSurface_Reachability
{
    return ck::nav_surface_recast::Get_IsReachable(
        ck_nav_surface_utils::Get_World(InWorldContext), InQuery).Get_Reachability();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Request_AreaMarkup(
        const UObject* InWorldContext,
        const FCk_Request_NavSurface_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_NavSurfaceMarkup
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    const auto WorldEntityIsValid = ck::IsValid(WorldEntity);
    CK_ENSURE_IF_NOT(WorldEntityIsValid,
        TEXT("Request_AreaMarkup could not resolve an ECS world from context object [{}]"),
        ck::IsValid(InWorldContext) ? InWorldContext->GetName() : FString{TEXT("NULL")})
    {
        InDelegate.ExecuteIfBound({}, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto AreaTagIsValid = InRequest.Get_AreaTag().IsValid();
    CK_ENSURE_IF_NOT(AreaTagIsValid,
        TEXT("Request_AreaMarkup was given no area tag"))
    {
        InDelegate.ExecuteIfBound(WorldEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    auto MarkupEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(WorldEntity);
    MarkupEntity.Add<ck::FFragment_NavSurfaceMarkup_Current>();

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& Requests = MarkupEntity.AddOrGet<ck::FFragment_NavSurfaceMarkup_Requests>();
    Requests._Requests.Add(InRequest);

    return CastChecked(MarkupEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Request_ImpassableBox(
        const UObject* InWorldContext,
        const FCk_Request_NavSurface_AreaMarkup& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_NavSurfaceMarkup
{
    auto ImpassableRequest = FCk_Request_NavSurface_AreaMarkup
    {
        InRequest.Get_Shape(),
        TAG_Nav_Area_Impassable.GetTag()
    };
    ImpassableRequest.Set_Enable(InRequest.Get_Enable());
    ImpassableRequest.Set_WorldTransform(InRequest.Get_WorldTransform());

    return Request_AreaMarkup(InWorldContext, ImpassableRequest, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_IsMarkupLive(
        const FCk_Handle_NavSurfaceMarkup& InMarkup)
    -> bool
{
    if (ck::Is_NOT_Valid(InMarkup))
    { return false; }

    const auto& Current = InMarkup.Get<ck::FFragment_NavSurfaceMarkup_Current>();
    if (NOT Current.Get_Markup().IsValid())
    { return false; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InMarkup);
    return ck::nav_surface_recast::Get_IsAreaLiveAt(
        World, Current.Get_AreaTag(), Current.Get_Location(), Current.Get_HalfExtents());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_SurfaceRevision(
        const UObject* InWorldContext)
    -> int64
{
    return ck::nav_surface_recast::Get_SurfaceRevision(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_SurfaceBounds(
        const UObject* InWorldContext)
    -> FBox
{
    return ck::nav_surface_recast::Get_SurfaceBounds(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_ProviderHealth(
        const UObject* InWorldContext)
    -> ECk_NavSurface_ProviderHealth
{
    return ck::nav_surface_recast::Get_ProviderHealth(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_IsBuildInProgress(
        const UObject* InWorldContext)
    -> bool
{
    return ck::nav_surface_recast::Get_IsBuildInProgress(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Request_SurfaceRebuild_ForTesting(
        const UObject* InWorldContext,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    if (NOT ck::nav_surface_recast::Request_SurfaceRebuild(ck_nav_surface_utils::Get_World(InWorldContext)))
    {
        InDelegate.ExecuteIfBound(WorldEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    InDelegate.ExecuteIfBound(WorldEntity, ECk_Request_OperationResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    BindTo_OnSurfaceRebuilt(
        const UObject* InWorldContext,
        const FCk_Delegate_NavSurface_OnSurfaceRebuilt& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    const auto WorldEntityIsValid = ck::IsValid(WorldEntity);
    CK_ENSURE_IF_NOT(WorldEntityIsValid,
        TEXT("BindTo_OnSurfaceRebuilt could not resolve an ECS world from context object [{}]"),
        ck::IsValid(InWorldContext) ? InWorldContext->GetName() : FString{TEXT("NULL")})
    { return; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_NavSurface_OnSurfaceRebuilt,
        WorldEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    UnbindFrom_OnSurfaceRebuilt(
        const UObject* InWorldContext,
        const FCk_Delegate_NavSurface_OnSurfaceRebuilt& InDelegate)
    -> void
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    const auto WorldEntityIsValid = ck::IsValid(WorldEntity);
    CK_ENSURE_IF_NOT(WorldEntityIsValid,
        TEXT("UnbindFrom_OnSurfaceRebuilt could not resolve an ECS world from context object [{}]"),
        ck::IsValid(InWorldContext) ? InWorldContext->GetName() : FString{TEXT("NULL")})
    { return; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_NavSurface_OnSurfaceRebuilt, WorldEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
