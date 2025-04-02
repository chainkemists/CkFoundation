#include "CkRaySense_Utils.h"

#include "CkRaySense/CkRaySense_Fragment.h"
#include "CkRaySense/CkRaySense_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RaySense_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_RaySense_ParamsData& InParams)
    -> FCk_Handle_RaySense
{
    InHandle.Add<ck::FFragment_RaySense_Params>(InParams);
    InHandle.Add<ck::FFragment_RaySense_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RaySense_UE, FCk_Handle_RaySense,
    ck::FFragment_RaySense_Params, ck::FFragment_RaySense_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RaySense_UE::
    Request_ExampleRequest(
        FCk_Handle_RaySense& InRaySense,
        const FCk_Request_RaySense_ExampleRequest& InRequest)
    -> FCk_Handle_RaySense
{
    InRaySense.AddOrGet<ck::FFragment_RaySense_Requests>()._Requests.Emplace(InRequest);
    return InRaySense;
}

auto
    UCk_Utils_RaySense_UE::
    BindTo_OnTraceHit(
        FCk_Handle_RaySense& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_RaySense_LineTrace& InDelegate)
    -> FCk_Handle_RaySense
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnRaySenseTraceHit, InHandle, InDelegate, InBehavior, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_RaySense_UE::
    UnbindFrom_OnTraceHit(
        FCk_Handle_RaySense& InHandle,
        const FCk_Delegate_RaySense_LineTrace& InDelegate)
    -> FCk_Handle_RaySense
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnRaySenseTraceHit, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
