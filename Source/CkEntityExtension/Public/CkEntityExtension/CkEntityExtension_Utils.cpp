#include "CkEntityExtension_Utils.h"

#include "CkEntityExtension/CkEntityExtension_Fragment.h"
#include "CkEntityExtension/CkEntityExtension_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityExtension_UE::
    Add(
        FCk_Handle& InExtensionOwner,
        FCk_Handle& InEntityToAddAsExtension,
        ECk_Replication InReplicates)
    -> FCk_Handle_EntityExtension
{
    RecordOfEntityExtensions_Utils::AddIfMissing(InExtensionOwner);
    InEntityToAddAsExtension.Add<ck::FFragment_EntityExtension_Params>(InExtensionOwner);

    auto EntityExtension = CastChecked(InEntityToAddAsExtension);

    RecordOfEntityExtensions_Utils::Request_Connect(InExtensionOwner, EntityExtension);

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

// --------------------------------------------------------------------------------------------------------------------
