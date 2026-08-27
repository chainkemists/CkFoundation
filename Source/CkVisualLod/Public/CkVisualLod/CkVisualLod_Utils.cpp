#include "CkVisualLod_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VisualLod_ParamsData& InParams)
    -> FCk_Handle_VisualLod
{
    InHandle.Add<ck::FFragment_VisualLod_Params>(InParams);
    auto& Current = InHandle.Add<ck::FFragment_VisualLod_Current>();
    Current._FarAnim = InParams.Get_InitialFarAnim();

    InHandle.Add<ck::FTag_VisualLod_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VisualLod_UE, FCk_Handle_VisualLod, ck::FFragment_VisualLod_Current, ck::FFragment_VisualLod_Params);

auto
    UCk_Utils_VisualLod_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    Get_Representation(
        const FCk_Handle_VisualLod& InHandle)
    -> ECk_VisualLod_Representation
{
    const auto& Current = InHandle.Get<ck::FFragment_VisualLod_Current>();

    if (Current.Get_Promoted())
    { return ECk_VisualLod_Representation::PromotedProxy; }

    if (Current.Get_MemberIndex() != INDEX_NONE)
    { return ECk_VisualLod_Representation::FarMember; }

    return ECk_VisualLod_Representation::None;
}

auto
    UCk_Utils_VisualLod_UE::
    Get_IsHidden(
        const FCk_Handle_VisualLod& InHandle)
    -> bool
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_Hidden();
}

auto
    UCk_Utils_VisualLod_UE::
    Get_FadeAlpha(
        const FCk_Handle_VisualLod& InHandle)
    -> float
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_FadeAlpha();
}

auto
    UCk_Utils_VisualLod_UE::
    Get_PromoteLockCount(
        const FCk_Handle_VisualLod& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_PromoteLock();
}

auto
    UCk_Utils_VisualLod_UE::
    Get_MemberIndex(
        const FCk_Handle_VisualLod& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_MemberIndex();
}

auto
    UCk_Utils_VisualLod_UE::
    Get_Crowd(
        const FCk_Handle_VisualLod& InHandle)
    -> ACk_Iskm_BatchedCrowd_Actor*
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_Crowd().Get();
}

auto
    UCk_Utils_VisualLod_UE::
    TryGet_Proxy(
        const FCk_Handle_VisualLod& InHandle)
    -> FCk_Handle_IskmProxy
{
    return InHandle.Get<ck::FFragment_VisualLod_Current>().Get_Proxy();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    Request_SetArbiter(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetArbiter& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_SetVisibility(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetVisibility& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_SetFarAnim(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetFarAnim& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_SetRenderer(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetRenderer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_Suspend(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_Suspend& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_Resume(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_Resume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLod_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLod_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    Request_AcquirePromoteLock(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    auto& Current = InHandle.Get<ck::FFragment_VisualLod_Current>();
    Current._PromoteLock = Current._PromoteLock + 1;

    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    Request_ReleasePromoteLock(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLod
{
    auto& Current = InHandle.Get<ck::FFragment_VisualLod_Current>();
    Current._PromoteLock = FMath::Max(Current._PromoteLock - 1, 0);

    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    BindTo_OnMemberAcquired(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLod_MemberAcquired, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    BindTo_OnPromoted(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_Promoted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLod_Promoted, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    BindTo_OnDemoteFinishing(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLod_DemoteFinishing, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    BindTo_OnMemberReleased(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLod_MemberReleased, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLod_UE::
    UnbindFrom_OnMemberAcquired(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLod_MemberAcquired, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    UnbindFrom_OnPromoted(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_Promoted& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLod_Promoted, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    UnbindFrom_OnDemoteFinishing(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLod_DemoteFinishing, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_VisualLod_UE::
    UnbindFrom_OnMemberReleased(
        FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate)
    -> FCk_Handle_VisualLod
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLod_MemberReleased, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
