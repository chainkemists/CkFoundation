#include "CkVatProxy_Utils.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VatProxy_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VatProxy_ParamsData& InParams)
    -> FCk_Handle_VatProxy
{
    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Entity [{}] already has a Vat — one Vat per entity"), InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Collection()),
        TEXT("Cannot add Vat to entity [{}] — the VatCollection is invalid"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_VatProxy_Params>(InParams);
    InHandle.Add<ck::FFragment_VatProxy_Current>();
    InHandle.Add<ck::FTag_VatProxy_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VatProxy_UE, FCk_Handle_VatProxy, ck::FFragment_VatProxy_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VatProxy_UE::
    Request_PlayClip(
        FCk_Handle_VatProxy& InHandle,
        const FCk_Request_VatProxy_PlayClip& InRequest)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_VatProxy_UE::
    Request_Stop(
        FCk_Handle_VatProxy& InHandle)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(FCk_Request_VatProxy_Stop{});
    return InHandle;
}

auto
    UCk_Utils_VatProxy_UE::
    Request_SetPlayRate(
        FCk_Handle_VatProxy& InHandle,
        float InPlayRate)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InHandle; }
    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(FCk_Request_VatProxy_SetPlayRate{InPlayRate});
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VatProxy_UE::
    Get_Collection(
        const FCk_Handle_VatProxy& InHandle)
    -> UCk_VatCollection_Data*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }
    return InHandle.Get<ck::FFragment_VatProxy_Params>().Get_Collection();
}

auto
    UCk_Utils_VatProxy_UE::
    Get_ActiveClipName(
        const FCk_Handle_VatProxy& InHandle)
    -> FName
{
    if (ck::Is_NOT_Valid(InHandle))
    { return {}; }

    const auto& Current = InHandle.Get<ck::FFragment_VatProxy_Current>();
    if (Current.Get_ActiveClipIndex() == INDEX_NONE)
    { return {}; }

    const auto& Collection = InHandle.Get<ck::FFragment_VatProxy_Params>().Get_Collection();
    if (ck::Is_NOT_Valid(Collection))
    { return {}; }

    const auto& BakedClips = Collection->Get_BakedClips();
    if (NOT BakedClips.IsValidIndex(Current.Get_ActiveClipIndex()))
    { return {}; }

    return BakedClips[Current.Get_ActiveClipIndex()].Get_Name();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VatProxy_UE::
    BindTo_OnClipFinished(
        FCk_Handle_VatProxy& InHandle,
        const FCk_Delegate_VatProxy_OnClipFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VatProxy
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_VatProxy_OnClipFinished,
        InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VatProxy_UE::
    UnbindFrom_OnClipFinished(
        FCk_Handle_VatProxy& InHandle,
        const FCk_Delegate_VatProxy_OnClipFinished& InDelegate)
    -> FCk_Handle_VatProxy
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_VatProxy_OnClipFinished, InHandle, InDelegate);
    return InHandle;
}
