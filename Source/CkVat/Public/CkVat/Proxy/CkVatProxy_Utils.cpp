#include "CkVatProxy_Utils.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_proxy_utils
{
    // Resident at Add by contract, so this batch completes inline — it exists to ROOT the resolved
    // collection for the entity's lifetime, not to load it.
    static const auto PinConsumerId = FName{TEXT("VatProxy.CollectionPin")};
}

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

    // Residency is the caller's contract — this diagnoses the misuse.
    const auto CollectionIsResident = ck::IsValid(InParams.Get_Collection().Get());
    CK_ENSURE_IF_NOT(CollectionIsResident,
        TEXT("Cannot add Vat to entity [{}] — the VatCollection [{}] is unset or NOT RESIDENT. "
             "It must be loaded AND baked before Add — async-load the soft reference yourself."),
        InHandle, InParams.Get_Collection().ToSoftObjectPath().ToString())
    { return {}; }

    InHandle.Add<ck::FFragment_VatProxy_Params>(InParams);
    auto& Current = InHandle.Add<ck::FFragment_VatProxy_Current>();
    InHandle.Add<ck::FTag_VatProxy_NeedsSetup>();

    Current._CollectionPinBatch = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
        ck_vat_proxy_utils::PinConsumerId, {InParams.Get_Collection().ToSoftObjectPath()});

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VatProxy_UE, FCk_Handle_VatProxy, ck::FFragment_VatProxy_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VatProxy_UE::
    Request_PlayClip(
        FCk_Handle_VatProxy& InHandle,
        const FCk_Request_VatProxy_PlayClip& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_VatProxy_UE::
    Request_Stop(
        FCk_Handle_VatProxy& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto Request = FCk_Request_VatProxy_Stop{};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_VatProxy_UE::
    Request_SetPlayRate(
        FCk_Handle_VatProxy& InHandle,
        float InPlayRate,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VatProxy
{
    if (ck::Is_NOT_Valid(InHandle))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    const auto Request = FCk_Request_VatProxy_SetPlayRate{InPlayRate};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VatProxy_Requests>()._Requests.Emplace(Request);
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
    return InHandle.Get<ck::FFragment_VatProxy_Params>().Get_Collection().Get();
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

    const auto* Collection = InHandle.Get<ck::FFragment_VatProxy_Params>().Get_Collection().Get();
    if (ck::Is_NOT_Valid(Collection))
    { return {}; }

    const auto& BakedClips = Collection->Get_BakedData().Get_BakedClips();
    CK_ENSURE_IF_NOT(BakedClips.IsValidIndex(Current.Get_ActiveClipIndex()),
        TEXT("Vat entity [{}]: active clip index [{}] is out of range of the collection's baked clip table (Num [{}])"),
        InHandle, Current.Get_ActiveClipIndex(), BakedClips.Num())
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
