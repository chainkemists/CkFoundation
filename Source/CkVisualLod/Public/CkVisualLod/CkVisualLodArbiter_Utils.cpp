#include "CkVisualLodArbiter_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"

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

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_Crowd(
        const FCk_Handle_VisualLodArbiter& InHandle,
        int32 InCrowdIndex)
    -> ACk_Iskm_BatchedCrowd_Actor*
{
    const auto& Current = InHandle.Get<ck::FFragment_VisualLodArbiter_Current>();

    if (NOT Current._Crowds.IsValidIndex(InCrowdIndex))
    { return nullptr; }

    return Current._Crowds[InCrowdIndex]._Crowd.Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_LastView(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> ck::FVisualLod_LocalView
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_LastView();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PromotesThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PromotesThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_DemotesThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_DemotesThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PreemptsThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PreemptsThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_IsFrozen(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_VisualLodArbiter_Frozen>();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_SetFrozen(
        FCk_Handle_VisualLodArbiter& InHandle,
        bool InFrozen)
    -> void
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    const auto Request = FCk_Request_VisualLodArbiter_SetFrozen{
        InFrozen ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable};

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(Request);
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_NumCrowds(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>()._Crowds.Num();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_CrowdPoolDebugInfo(
        const FCk_Handle_VisualLodArbiter& InHandle,
        int32 InCrowdIndex)
    -> FCk_VisualLodArbiter_CrowdPoolDebugInfo
{
    const auto& Current = InHandle.Get<ck::FFragment_VisualLodArbiter_Current>();

    if (NOT Current._Crowds.IsValidIndex(InCrowdIndex))
    { return {}; }

    const auto& Runtime = Current._Crowds[InCrowdIndex];

    auto Info = FCk_VisualLodArbiter_CrowdPoolDebugInfo{};
    Info.PoolSize  = Runtime._SlotOwners.Num();
    Info.FreeSlots = Runtime._FreeSlots.Num();
    Info.Crowd     = Runtime._Crowd;

    return Info;
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

auto
    UCk_Utils_VisualLodArbiter_UE::
    BindTo_OnCrowdCreated(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLodArbiter
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLodArbiter_CrowdCreated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    UnbindFrom_OnCrowdCreated(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLodArbiter_CrowdCreated, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
