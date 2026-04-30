#include "CkNav_Utils.h"

#include "CkNavigation/CkNavigation_Log.h"

#include "CkEcs/Signal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_UE::
    Request_FindPath(
        FCk_Handle& InHandle,
        const FCk_Request_Nav_FindPath& InRequest)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Invalid handle [{}] passed to UCk_Utils_Nav_UE::Request_FindPath"), InHandle)
    { return InHandle; }

    // Make sure the result fragment exists so consumers can read PathResult immediately
    // (before the processor drains the request, Status will be None — Pending becomes
    // visible after the processor's next ForEachEntity tick).
    auto& Result = InHandle.AddOrGet<ck::FFragment_Nav_PathResult>();

    // Append to the per-entity request queue. The processor's view membership is keyed
    // on the existence of FFragment_Nav_Requests, so AddOrGet is what re-arms the dirty
    // event when the queue had been drained empty in a prior tick.
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
