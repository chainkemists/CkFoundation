#include "CkEntityCollection_Fragment.h"

#include "CkEntityCollection/CkEntityCollection_Log.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include <Net/Core/PushModel/PushModel.h>
#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityCollection_Rep::
    Broadcast_AddOrUpdate(
        const FCk_EntityCollection_Content& InEntityCollectionContent)
    -> void
{
    const auto Found = _EntityCollectionsToReplicate.FindByPredicate([&](const FCk_EntityCollection_Content& InElement)
    {
        return InElement.Get_CollectionName() == InEntityCollectionContent.Get_CollectionName();
    });

    if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
    {
        _EntityCollectionsToReplicate.Emplace(InEntityCollectionContent);
    }
    else
    {
        *Found = InEntityCollectionContent;
    }

    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _EntityCollectionsToReplicate, this);
}

auto
    UCk_Fragment_EntityCollection_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _EntityCollectionsToReplicate, Params);
}

auto
    UCk_Fragment_EntityCollection_Rep::
    PostLink()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_EntityCollection_Rep::
    Request_TryUpdateReplicatedEntityCollections()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_EntityCollection_Rep::
    OnRep_Updated()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    for (auto Index = _EntityCollectionsToReplicate_Previous.Num(); Index < _EntityCollectionsToReplicate.Num(); ++Index)
    {
        const auto& EntityCollectionToReplicate = _EntityCollectionsToReplicate[Index];

        if (const auto& EntityCollectionEntity = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(Get_AssociatedEntity(), EntityCollectionToReplicate.Get_CollectionName());
            ck::Is_NOT_Valid(EntityCollectionEntity))
        {
            ck::entity_collection::Verbose(TEXT("Could NOT find EntityCollection [{}]. EntityCollection replication PENDING..."),
                EntityCollectionToReplicate.Get_CollectionName());

            return;
        }

        const auto AllValidEntities = ck::algo::AllOf(EntityCollectionToReplicate.Get_EntitiesInCollection(), ck::algo::IsValidEntityHandle{});

        ck::entity_collection::VerboseIf(NOT AllValidEntities, TEXT("At least one invalid entity in EntityCollection [{}]. EntityCollection replication PENDING..."),
            EntityCollectionToReplicate.Get_CollectionName());

        if (NOT AllValidEntities)
        { return; }
    }

    for (auto Index = 0; Index < _EntityCollectionsToReplicate.Num(); ++Index)
    {
        const auto& EntityCollectionToReplicate = _EntityCollectionsToReplicate[Index];
        auto EntityCollectionEntity = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(Get_AssociatedEntity(), EntityCollectionToReplicate.Get_CollectionName());
        const auto& CurrentCollectionContent = UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(EntityCollectionEntity);

        if (NOT _EntityCollectionsToReplicate_Previous.IsValidIndex(Index))
        {
            ck::entity_collection::Verbose(TEXT("Replicating EntityCollection for the FIRST time to [{}]"), EntityCollectionToReplicate);

            UCk_Utils_EntityCollection_UE::Request_RemoveEntities(EntityCollectionEntity, FCk_Request_EntityCollection_RemoveEntities{CurrentCollectionContent.Get_EntitiesInCollection()});
            UCk_Utils_EntityCollection_UE::Request_AddEntities(EntityCollectionEntity, FCk_Request_EntityCollection_AddEntities{EntityCollectionToReplicate.Get_EntitiesInCollection()});

            continue;
        }

        if (_EntityCollectionsToReplicate_Previous[Index] != EntityCollectionToReplicate)
        {
            ck::entity_collection::Verbose(TEXT("Replicating EntityCollection and UPDATING it to [{}]"), EntityCollectionToReplicate);

            UCk_Utils_EntityCollection_UE::Request_RemoveEntities(EntityCollectionEntity, FCk_Request_EntityCollection_RemoveEntities{CurrentCollectionContent.Get_EntitiesInCollection()});
            UCk_Utils_EntityCollection_UE::Request_AddEntities(EntityCollectionEntity, FCk_Request_EntityCollection_AddEntities{EntityCollectionToReplicate.Get_EntitiesInCollection()});

            continue;
        }

        ck::entity_collection::Verbose(TEXT("IGNORING EntityCollection [{}] as there is no change between [{}] and [{}]"),
            EntityCollectionToReplicate.Get_CollectionName(),
            _EntityCollectionsToReplicate_Previous[Index],
            EntityCollectionToReplicate);
    }

    _EntityCollectionsToReplicate_Previous = _EntityCollectionsToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
