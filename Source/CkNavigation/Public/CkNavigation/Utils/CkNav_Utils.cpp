#include "CkNav_Utils.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/CkNavigation_Stats.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include <NavigationSystem.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Nav::ProjectOntoNavmesh"), STAT_Nav_ProjectOntoNavmesh, STATGROUP_CkNav);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Request_FindPath(
        FCk_Handle& InHandle,
        const FCk_Request_Nav_FindPath& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid handle [{}] passed to UCk_Utils_Nav_UE::Request_FindPath"), InHandle)
    {}
    if (NOT HandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    auto& Result = InHandle.AddOrGet<ck::FFragment_Nav_PathResult>();

    // AddOrGet, not Get: the processor's view membership is keyed on the fragment existing,
    // so re-adding it is what re-arms the dirty event after a prior tick drained it away.
    auto& Requests = InHandle.AddOrGet<ck::FFragment_Nav_Requests>();
    Requests._Requests.Add(InRequest);

    ck::nav::Verbose(TEXT("FindPath enqueued on [{}] -> target [{}]"),
        InHandle, InRequest.Get_TargetLocation());

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Get_PathResult(
        const FCk_Handle& InHandle)
    -> FCk_Nav_PathResult
{
    if (NOT ck::IsValid(InHandle))
    { return {}; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return {}; }

    return InHandle.Get<ck::FFragment_Nav_PathResult>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Get_PathStatus(
        const FCk_Handle& InHandle)
    -> ECk_Nav_PathStatus
{
    if (NOT ck::IsValid(InHandle))
    { return ECk_Nav_PathStatus::None; }

    if (NOT InHandle.Has<ck::FFragment_Nav_PathResult>())
    { return ECk_Nav_PathStatus::None; }

    return InHandle.Get<ck::FFragment_Nav_PathResult>().Get_Status();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Has_Path(
        const FCk_Handle& InHandle)
    -> bool
{
    return Get_PathStatus(InHandle) == ECk_Nav_PathStatus::Ready
        || Get_PathStatus(InHandle) == ECk_Nav_PathStatus::Partial;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Request_NavigationRebuild_ForTesting(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Invalid handle [{}] passed to Request_NavigationRebuild_ForTesting"), InHandle)
    {}
    if (NOT HandleIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if (NOT IsValid(World))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    auto* NavSys = UNavigationSystemV1::GetCurrent(World);
    if (NavSys == nullptr)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    NavSys->Build();
    ck::nav::Verbose(TEXT("Request_NavigationRebuild_ForTesting kicked off Build() on world [{}]"), World);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Try_ProjectOntoNavmesh(
        FCk_Handle& InHandle,
        FVector InWorldPosition,
        float InHalfExtentUu,
        FVector& OutSnappedPosition,
        float InVerticalHalfExtentUu)
    -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_Nav_ProjectOntoNavmesh);

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid handle [{}] passed to Try_ProjectOntoNavmesh"), InHandle)
    { return false; }

    auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if (NOT ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    auto* NavSys = UNavigationSystemV1::GetCurrent(World);
    if (NavSys == nullptr)
    { return false; }

    const auto HalfExt = FMath::Max(InHalfExtentUu, 1.0f);
    const auto UseUniformCube = InVerticalHalfExtentUu < 0.0f;
    const auto VertHalfExt = UseUniformCube ? HalfExt : FMath::Max(InVerticalHalfExtentUu, 1.0f);
    const auto ProjectionExtent = FVector{HalfExt, HalfExt, VertHalfExt};

    auto Projected = FNavLocation{};
    const auto bSuccess = NavSys->ProjectPointToNavigation(InWorldPosition, Projected, ProjectionExtent);
    if (bSuccess)
    { OutSnappedPosition = Projected.Location; }

    return bSuccess;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    BindTo_OnPathReady(
        FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Nav_OnPathReady, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    UnbindFrom_OnPathReady(
        FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathReady& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Nav_OnPathReady, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    BindTo_OnPathFailed(
        FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Nav_OnPathFailed, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Nav_UE::
    UnbindFrom_OnPathFailed(
        FCk_Handle& InHandle,
        const FCk_Delegate_Nav_OnPathFailed& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Nav_OnPathFailed, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
