#include "CkVisualLodArbiter_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VisualLodArbiter_ParamsData& InParams)
    -> FCk_Handle_VisualLodArbiter
{
    InHandle.Add<ck::FFragment_VisualLodArbiter_Params>(InParams);
    InHandle.Add<ck::FFragment_VisualLodArbiter_Current>();

    InHandle.Add<ck::FTag_VisualLodArbiter_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VisualLodArbiter_UE, FCk_Handle_VisualLodArbiter, ck::FFragment_VisualLodArbiter_Current, ck::FFragment_VisualLodArbiter_Params);

auto
    UCk_Utils_VisualLodArbiter_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_Observer(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_Observer();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PromotedOwners().Num();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_NearPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_NearPromotedCount();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_LockedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_LockedPromotedCount();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_UnbudgetedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_UnbudgetedPromotedCount();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_SetObserver(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_SetObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_ClearObserver(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_ClearObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
