#include "CkEntityCollection_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityCollection_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_EntityCollection_ParamsData& InParams)
    -> FCk_Handle_EntityCollection
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());
        InNewEntity.Add<ck::FFragment_EntityCollection_Params>(InParams);
        EntityCollections_RecordOfEntities_Utils::AddIfMissing(InNewEntity);
    });

    auto NewEntityCollectionEntity = Cast(NewEntity);

    RecordOfEntityCollections_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfEntityCollections_Utils::Request_Connect(InHandle, NewEntityCollectionEntity);

    return NewEntityCollectionEntity;
}

auto
    UCk_Utils_EntityCollection_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleEntityCollection_ParamsData& InParams)
    -> TArray<FCk_Handle_EntityCollection>
{
    return ck::algo::Transform<TArray<FCk_Handle_EntityCollection>>(InParams.Get_EntityCollectionParams(),
    [&](const FCk_Fragment_EntityCollection_ParamsData& InEntityCollectionParams)
    {
        return Add(InHandle, InEntityCollectionParams);
    });
}

auto
    UCk_Utils_EntityCollection_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfEntityCollections_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(EntityCollection, UCk_Utils_EntityCollection_UE, FCk_Handle_EntityCollection, ck::FFragment_EntityCollection_Params);

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
    -> TArray<FCk_Handle>
{
    return EntityCollections_RecordOfEntities_Utils::Get_ValidEntries(InEntityCollectionHandle);
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

// --------------------------------------------------------------------------------------------------------------------
