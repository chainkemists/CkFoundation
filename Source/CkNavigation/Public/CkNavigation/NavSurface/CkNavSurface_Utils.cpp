#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/NavSurface/CkNavSurface_GameplayTags.h"
#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"

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

    // Which table answers this context object, or nullptr when the resolved provider has registered
    // none. A null world is NOT special-cased: the project default still resolves, and every provider
    // is required to answer a world-less call with its own no-provider shape.
    auto TryGet_ProviderTable(UWorld* InWorld) -> const FCk_NavSurface_ProviderTable*
    {
        return ck::nav_surface::TryGet_ProviderTable(ck::nav_surface::Get_ProviderForWorld(InWorld));
    }

    // One helper per return shape rather than a bare `return {}` at each call site: the no-provider
    // answer is part of the contract, and naming it keeps every capability agreeing on what it is.
    auto Get_NoProvider_Projection() -> FCk_NavSurface_ProjectionResult
    {
        auto Result = FCk_NavSurface_ProjectionResult{};
        Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
        return Result;
    }

    auto Get_NoProvider_MoveAlongSurface() -> FCk_NavSurface_MoveAlongSurfaceResult
    {
        auto Result = FCk_NavSurface_MoveAlongSurfaceResult{};
        Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
        return Result;
    }

    auto Get_NoProvider_Raycast() -> FCk_NavSurface_RaycastResult
    {
        auto Result = FCk_NavSurface_RaycastResult{};
        Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
        return Result;
    }

    auto Get_NoProvider_Boundary() -> FCk_NavSurface_BoundaryResult
    {
        auto Result = FCk_NavSurface_BoundaryResult{};
        Result.Set_Status(ECk_NavSurface_QueryStatus::NoProvider);
        return Result;
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
    Request_SetProvider(
        const UObject* InWorldContext,
        ECk_NavSurface_Provider InProvider)
    -> void
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    const auto WorldEntityIsValid = ck::IsValid(WorldEntity);
    CK_ENSURE_IF_NOT(WorldEntityIsValid,
        TEXT("Request_SetProvider could not resolve an ECS world from context object [{}]"),
        ck::IsValid(InWorldContext) ? InWorldContext->GetName() : FString{TEXT("NULL")})
    { return; }

    auto& Provider = WorldEntity.AddOrGet<ck::FFragment_NavSurface_Provider>();
    Provider._Provider = InProvider;

    ck::nav_surface::Set_ProviderForWorld(ck_nav_surface_utils::Get_World(InWorldContext), InProvider);

    ck::nav::Display
    (
        TEXT("NavSurface provider for world entity [{}] set to [{}]"),
        WorldEntity, InProvider
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_Provider(
        const UObject* InWorldContext)
    -> ECk_NavSurface_Provider
{
    return ck::nav_surface::Get_ProviderForWorld(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Request_SetShadowMode(
        const UObject* InWorldContext,
        ECk_NavSurface_ShadowMode InMode)
    -> void
{
    auto WorldEntity = ck_nav_surface_utils::Get_WorldEntity(InWorldContext);

    const auto WorldEntityIsValid = ck::IsValid(WorldEntity);
    CK_ENSURE_IF_NOT(WorldEntityIsValid,
        TEXT("Request_SetShadowMode could not resolve an ECS world from context object [{}]"),
        ck::IsValid(InWorldContext) ? InWorldContext->GetName() : FString{TEXT("NULL")})
    { return; }

    auto& Provider = WorldEntity.AddOrGet<ck::FFragment_NavSurface_Provider>();
    Provider._ShadowMode = InMode;

    ck::nav_surface::Set_ShadowModeForWorld(ck_nav_surface_utils::Get_World(InWorldContext), InMode);

    ck::nav::Display
    (
        TEXT("NavSurface shadow mode for world entity [{}] set to [{}]"),
        WorldEntity, InMode
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_ShadowMode(
        const UObject* InWorldContext)
    -> ECk_NavSurface_ShadowMode
{
    return ck::nav_surface::Get_ShadowModeForWorld(ck_nav_surface_utils::Get_World(InWorldContext));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_ProjectPoint(
        const UObject* InWorldContext,
        const FCk_NavSurface_ProjectionQuery& InQuery)
    -> FCk_NavSurface_ProjectionResult
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return ck_nav_surface_utils::Get_NoProvider_Projection(); }

    return Table->_ProjectPoint(World, InQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_MoveAlongSurface(
        const UObject* InWorldContext,
        const FCk_NavSurface_MoveAlongSurfaceQuery& InQuery)
    -> FCk_NavSurface_MoveAlongSurfaceResult
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return ck_nav_surface_utils::Get_NoProvider_MoveAlongSurface(); }

    return Table->_MoveAlongSurface(World, InQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Try_SurfaceRaycast(
        const UObject* InWorldContext,
        const FCk_NavSurface_RaycastQuery& InQuery)
    -> FCk_NavSurface_RaycastResult
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return ck_nav_surface_utils::Get_NoProvider_Raycast(); }

    return Table->_SurfaceRaycast(World, InQuery);
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
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);

    const auto Result = Table == nullptr
        ? ck_nav_surface_utils::Get_NoProvider_Boundary()
        : Table->_BoundarySegments(World, InQuery);

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
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return ECk_NavSurface_Reachability::Unknown_ProviderNotReady; }

    return Table->_IsReachable(World, InQuery).Get_Reachability();
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

    auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InMarkup);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return false; }

    return Table->_IsMarkupLive(World, InMarkup);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_SurfaceRevision(
        const UObject* InWorldContext)
    -> int64
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return 0; }

    return Table->_SurfaceRevision(World);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_SurfaceBounds(
        const UObject* InWorldContext)
    -> FBox
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return FBox{ForceInit}; }

    return Table->_SurfaceBounds(World);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_ProviderHealth(
        const UObject* InWorldContext)
    -> ECk_NavSurface_ProviderHealth
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return ECk_NavSurface_ProviderHealth::NoData; }

    return Table->_ProviderHealth(World);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NavSurface_UE::
    Get_IsBuildInProgress(
        const UObject* InWorldContext)
    -> bool
{
    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);
    if (Table == nullptr)
    { return false; }

    return Table->_IsBuildInProgress(World);
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

    auto* World = ck_nav_surface_utils::Get_World(InWorldContext);

    const auto* Table = ck_nav_surface_utils::TryGet_ProviderTable(World);

    if (Table == nullptr || NOT Table->_RequestSurfaceRebuild(World))
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
