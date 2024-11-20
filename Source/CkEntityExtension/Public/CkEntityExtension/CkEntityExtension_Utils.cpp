#include "CkEntityExtension_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Fragment.h"
#include "CkEntityExtension/CkEntityExtension_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityExtension_UE::
    Add(
        FCk_Handle& InExtensionOwner,
        FCk_Handle& InEntityToAddAsExtension)
    -> FCk_Handle_EntityExtension
{
    RecordOfEntityExtensions_Utils::AddIfMissing(InExtensionOwner);
    InEntityToAddAsExtension.Add<ck::FFragment_EntityExtension_Params>(InExtensionOwner);

    auto EntityExtension = CastChecked(InEntityToAddAsExtension);

    RecordOfEntityExtensions_Utils::Request_Connect(InExtensionOwner, EntityExtension);

    DoBoadcast_ExtensionAdded(InExtensionOwner, EntityExtension);

    return EntityExtension;
}

auto
    UCk_Utils_EntityExtension_UE::
    Remove(
        FCk_Handle& InExtensionOwner,
        FCk_Handle_EntityExtension& InEntityToRemoveAsExtension)
    -> FCk_Handle_EntityExtension
{
    RecordOfEntityExtensions_Utils::Request_Disconnect(InExtensionOwner, InEntityToRemoveAsExtension);
    InEntityToRemoveAsExtension.Remove<ck::FFragment_EntityExtension_Params>();

    DoBoadcast_ExtensionRemoved(InExtensionOwner, InEntityToRemoveAsExtension);

    return InEntityToRemoveAsExtension;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityExtension_UE, FCk_Handle_EntityExtension, ck::FFragment_EntityExtension_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityExtension_UE::
    Get_ExtensionOwner(
        const FCk_Handle_EntityExtension& InEntityExtension)
        -> FCk_Handle
{
    return InEntityExtension.Get<ck::FFragment_EntityExtension_Params>().Get_ExtensionOwner();
}

auto
    UCk_Utils_EntityExtension_UE::
    BindTo_OnExtensionAdded(
        FCk_Handle& InExtensionOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityExtension_OnExtensionAdded& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEntityExtensionAdded, InExtensionOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InExtensionOwner;
}

auto
    UCk_Utils_EntityExtension_UE::
    UnbindFrom_OnExtensionAdded(
        FCk_Handle& InExtensionOwner,
        const FCk_Delegate_EntityExtension_OnExtensionAdded& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEntityExtensionAdded, InExtensionOwner, InDelegate);
    return InExtensionOwner;
}

auto
    UCk_Utils_EntityExtension_UE::
    BindTo_OnExtensionRemoved(
        FCk_Handle& InExtensionOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityExtension_OnExtensionRemoved& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnEntityExtensionRemoved, InExtensionOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InExtensionOwner;
}

auto
    UCk_Utils_EntityExtension_UE::
    UnbindFrom_OnExtensionRemoved(
        FCk_Handle& InExtensionOwner,
        const FCk_Delegate_EntityExtension_OnExtensionRemoved& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnEntityExtensionRemoved, InExtensionOwner, InDelegate);
    return InExtensionOwner;
}

auto
    UCk_Utils_EntityExtension_UE::
    ForEach_EntityExtension(
        FCk_Handle& InEntityExtensionOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_EntityExtension>
{
    auto EntityExtensions = TArray<FCk_Handle_EntityExtension>{};

    ForEach_EntityExtension(InEntityExtensionOwner,
    [&](FCk_Handle_EntityExtension InEntityExtension)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InEntityExtension, InOptionalPayload); }
        else
        { EntityExtensions.Emplace(InEntityExtension); }
    });

    return EntityExtensions;
}

auto
    UCk_Utils_EntityExtension_UE::
    ForEach_EntityExtension(
        FCk_Handle& InEntityExtensionOwner,
        const TFunction<void(FCk_Handle_EntityExtension)>& InFunc)
    -> void
{
    RecordOfEntityExtensions_Utils::ForEach_ValidEntry(InEntityExtensionOwner, InFunc);
}

auto
    UCk_Utils_EntityExtension_UE::
    DoBoadcast_ExtensionAdded(
        const FCk_Handle& InExtensionOwner,
        const FCk_Handle_EntityExtension& InEntityExtension)
    -> void
{
    if (NOT RecordOfEntityExtensions_Utils::Has(InExtensionOwner))
    { return; }

    ck::UUtils_Signal_OnEntityExtensionAdded::Broadcast(InExtensionOwner, ck::MakePayload(InExtensionOwner, InEntityExtension));

    DoBoadcast_ExtensionAdded(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InExtensionOwner), InEntityExtension);
}

auto
    UCk_Utils_EntityExtension_UE::
    DoBoadcast_ExtensionRemoved(
        const FCk_Handle& InExtensionOwner,
        const FCk_Handle_EntityExtension& InEntityExtension)
    -> void
{
    if (NOT RecordOfEntityExtensions_Utils::Has(InExtensionOwner))
    { return; }

    ck::UUtils_Signal_OnEntityExtensionRemoved::Broadcast(InExtensionOwner, ck::MakePayload(InExtensionOwner, InEntityExtension));

    DoBoadcast_ExtensionRemoved(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InExtensionOwner), InEntityExtension);
}

// --------------------------------------------------------------------------------------------------------------------
