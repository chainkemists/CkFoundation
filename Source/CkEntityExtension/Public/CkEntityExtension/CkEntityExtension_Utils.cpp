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
    auto& Params = InEntityToAddAsExtension.AddOrGet<ck::FFragment_EntityExtension_Params>();
    Params = ck::FFragment_EntityExtension_Params{InExtensionOwner};

    auto EntityExtension = CastChecked(InEntityToAddAsExtension);

    RecordOfEntityExtensions_Utils::Request_Connect(InExtensionOwner, EntityExtension);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::entity_extension::VeryVerbose
        (
            TEXT("Skipping creation of EntityExtension Rep Fragment on Entity [{}] because it's set to [{}]"),
            InEntityToAddAsExtension,
            InReplicates
        );

        return EntityExtension;
    }

    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_EntityReplication(InExtensionOwner) == ECk_Replication::Replicates,
        TEXT("ExtensionOwner [{}] is NOT replicated and thus the Extension CANNOT be added to the Owner [{}] on remote machines"),
        InExtensionOwner, InEntityToAddAsExtension)
    { return EntityExtension; }

    TryAddReplicatedFragment<UCk_Fragment_EntityExtension_Rep>(InEntityToAddAsExtension);

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

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_EntityExtension_Rep>(
        InEntityToRemoveAsExtension, [&](UCk_Fragment_EntityExtension_Rep* InRepComp)
    {
    });

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
