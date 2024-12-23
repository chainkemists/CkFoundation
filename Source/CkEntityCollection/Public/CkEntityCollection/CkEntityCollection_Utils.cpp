#include "CkEntityCollection_Utils.h"

#include "CkEntityCollection/CkEntityCollection_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityCollection_UE::
    Add(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const FCk_Fragment_EntityCollection_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_EntityCollection
{
    auto NewEntityCollectionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_EntityCollection>(InEntityCollectionOwnerEntity);

    NewEntityCollectionEntity.Add<ck::FFragment_EntityCollection_Params>(InParams);
    UCk_Utils_GameplayLabel_UE::Add(NewEntityCollectionEntity, InParams.Get_Name());
    EntityCollections_RecordOfEntities_Utils::AddIfMissing(NewEntityCollectionEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::entity_collection::VeryVerbose
        (
            TEXT("Skipping creation of EntityCollection Rep Fragment on Entity [{}] because it's set to [{}]"),
            NewEntityCollectionEntity,
            InReplicates
        );
    }
    else
    {
        TryAddReplicatedFragment<UCk_Fragment_EntityCollection_Rep>(InEntityCollectionOwnerEntity);
    }

    RecordOfEntityCollections_Utils::AddIfMissing(InEntityCollectionOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfEntityCollections_Utils::Request_Connect(InEntityCollectionOwnerEntity, NewEntityCollectionEntity);

    // it's possible that we have pending replication info
    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InEntityCollectionOwnerEntity))
    {
        if (UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_EntityCollection_Rep>(InEntityCollectionOwnerEntity))
        {
            InEntityCollectionOwnerEntity.Try_Transform<TObjectPtr<UCk_Fragment_EntityCollection_Rep>>(
            [&](const TObjectPtr<UCk_Fragment_EntityCollection_Rep>& InRepComp)
            {
                InRepComp->Request_TryUpdateReplicatedEntityCollections();
            });
        }
    }

    return NewEntityCollectionEntity;
}

auto
    UCk_Utils_EntityCollection_UE::
    AddMultiple(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const FCk_Fragment_MultipleEntityCollection_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_EntityCollection>
{
    return ck::algo::Transform<TArray<FCk_Handle_EntityCollection>>(InParams.Get_EntityCollectionParams(),
    [&](const FCk_Fragment_EntityCollection_ParamsData& InEntityCollectionParams)
    {
        return Add(InEntityCollectionOwnerEntity, InEntityCollectionParams, InReplicates);
    });
}

auto
    UCk_Utils_EntityCollection_UE::
    Has_Any(
        const FCk_Handle& InEntityCollectionOwnerEntity)
    -> bool
{
    return RecordOfEntityCollections_Utils::Has(InEntityCollectionOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityCollection_UE, FCk_Handle_EntityCollection, ck::FFragment_EntityCollection_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityCollection_UE::
    TryGet_EntityCollection(
        const FCk_Handle& InEntityCollectionOwnerEntity,
        FGameplayTag InEntityCollectionName)
    -> FCk_Handle_EntityCollection
{
    return RecordOfEntityCollections_Utils::Get_ValidEntry_If(InEntityCollectionOwnerEntity, ck::algo::MatchesGameplayLabelExact{InEntityCollectionName});
}

auto
    UCk_Utils_EntityCollection_UE::
    Get_EntitiesInCollection(
        const FCk_Handle_EntityCollection& InEntityCollectionHandle)
    -> FCk_EntityCollection_Content
{
    const auto& CollectionName = InEntityCollectionHandle.Get<ck::FFragment_EntityCollection_Params>().Get_Params().Get_Name();
    const auto& CollectionContent = EntityCollections_RecordOfEntities_Utils::Get_ValidEntries(InEntityCollectionHandle);

    return FCk_EntityCollection_Content{CollectionName, CollectionContent};
}

auto
    UCk_Utils_EntityCollection_UE::
    ForEach_EntityCollection(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_EntityCollection>
{
    auto EntityCollections = TArray<FCk_Handle_EntityCollection>{};

    ForEach_EntityCollection(InEntityCollectionOwnerEntity, [&](FCk_Handle_EntityCollection InEntityCollection)
    {
        EntityCollections.Emplace(InEntityCollection);

        std::ignore = InDelegate.ExecuteIfBound(InEntityCollection, InOptionalPayload);
    });

    return EntityCollections;
}

auto
    UCk_Utils_EntityCollection_UE::
    ForEach_EntityCollection(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const TFunction<void(FCk_Handle_EntityCollection)>& InFunc)
    -> void
{
    RecordOfEntityCollections_Utils::ForEach_ValidEntry(InEntityCollectionOwnerEntity, InFunc);
}

auto
    UCk_Utils_EntityCollection_UE::
    ForEach_EntityCollection_If(
        FCk_Handle& InEntityCollectionOwnerEntity,
        const TFunction<void(FCk_Handle_EntityCollection)>& InFunc,
        const TFunction<bool(FCk_Handle_EntityCollection)>& InPredicate)
    -> void
{
    RecordOfEntityCollections_Utils::ForEach_ValidEntry_If(InEntityCollectionOwnerEntity, InFunc, InPredicate);
}

auto
    UCk_Utils_EntityCollection_UE::
    Request_AddEntities(
        FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Request_EntityCollection_AddEntities& InRequest)
    -> FCk_Handle_EntityCollection
{
    InEntityCollectionHandle.AddOrGet<ck::FFragment_EntityCollection_Requests>()._Requests.Emplace(InRequest);
    return InEntityCollectionHandle;
}

auto
    UCk_Utils_EntityCollection_UE::
    Request_RemoveEntities(
        FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Request_EntityCollection_RemoveEntities& InRequest)
    -> FCk_Handle_EntityCollection
{
    InEntityCollectionHandle.AddOrGet<ck::FFragment_EntityCollection_Requests>()._Requests.Emplace(InRequest);
    return InEntityCollectionHandle;
}

auto
    UCk_Utils_EntityCollection_UE::
    BindTo_OnCollectionUpdated(
        FCk_Handle_EntityCollection& InEntityCollectionHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityCollection_OnCollectionUpdated& InDelegate)
    -> FCk_Handle_EntityCollection
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_EntityCollection_OnCollectionUpdated, InEntityCollectionHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InEntityCollectionHandle;
}

auto
    UCk_Utils_EntityCollection_UE::
    UnbindFrom_OnCollectionUpdated(
        FCk_Handle_EntityCollection& InEntityCollectionHandle,
        const FCk_Delegate_EntityCollection_OnCollectionUpdated& InDelegate)
    -> FCk_Handle_EntityCollection
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_EntityCollection_OnCollectionUpdated, InEntityCollectionHandle, InDelegate);
    return InEntityCollectionHandle;
}

auto
    UCk_Utils_EntityCollection_UE::
    Request_CollectionUpdated(
        FCk_Handle_EntityCollection& InEntityCollectionHandle)
    -> void
{
    InEntityCollectionHandle.AddOrGet<ck::FTag_EntityCollection_CollectionUpdated>();
}

auto
    UCk_Utils_EntityCollection_UE::
    Request_StorePreviousCollection(
        FCk_Handle_EntityCollection& InEntityCollectionHandle)
    -> void
{
    InEntityCollectionHandle.Try_Remove<ck::FFragment_EntityCollections_RecordOfEntities_Previous>();
    EntityCollections_RecordOfEntities_Previous_Utils::AddIfMissing(InEntityCollectionHandle);

    EntityCollections_RecordOfEntities_Utils::ForEach_ValidEntry(InEntityCollectionHandle, [&](FCk_Handle InEntry)
    {
        EntityCollections_RecordOfEntities_Previous_Utils::Request_Connect(InEntityCollectionHandle, InEntry);
    });
}

auto
    UCk_Utils_EntityCollection_UE::
    Request_TryReplicateEntityCollection(
        FCk_Handle_EntityCollection& InEntityCollectionHandle)
    -> void
{
    InEntityCollectionHandle.AddOrGet<ck::FTag_EntityCollection_MayRequireReplication>();
}

// --------------------------------------------------------------------------------------------------------------------
