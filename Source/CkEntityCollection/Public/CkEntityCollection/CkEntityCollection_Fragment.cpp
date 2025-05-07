#include "CkEntityCollection_Fragment.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

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
    auto AssociatedEntity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(AssociatedEntity))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    const auto& EntityCollectionsToReplicate = _EntityCollectionsToReplicate;
    const auto& EntityCollectionsToReplicate_Previous = _EntityCollectionsToReplicate_Previous;

    for (auto Index = EntityCollectionsToReplicate_Previous.Num(); Index < EntityCollectionsToReplicate.Num(); ++Index)
    {
        const auto& EntityCollectionToReplicate = EntityCollectionsToReplicate[Index];

        if (const auto& EntityCollectionEntity = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(
                AssociatedEntity, EntityCollectionToReplicate.Get_CollectionName());
            ck::Is_NOT_Valid(EntityCollectionEntity))
        {
            ck::entity_collection::Verbose(TEXT("Could NOT find EntityCollection [{}]. EntityCollection replication PENDING..."),
                EntityCollectionToReplicate.Get_CollectionName());

            return;
        }

        const auto AllValidEntities = ck::algo::AllOf(EntityCollectionToReplicate.Get_EntitiesInCollection(), [](
            const FCk_Handle& MaybeValidHandle)
        {
            return ck::IsValid(MaybeValidHandle);
        });

        ck::entity_collection::VerboseIf(NOT AllValidEntities,
            TEXT("At least one invalid entity in EntityCollection [{}]. EntityCollection replication PENDING..."),
            EntityCollectionToReplicate.Get_CollectionName());

        if (NOT AllValidEntities)
        { return; }
    }

    if (EntityCollectionsToReplicate.Num() == EntityCollectionsToReplicate_Previous.Num())
    { return; }

    AssociatedEntity.AddOrGet<ck::FFragment_EntityCollection_SyncReplication>(_EntityCollectionsToReplicate,
        _EntityCollectionsToReplicate_Previous);

    _EntityCollectionsToReplicate_Previous = _EntityCollectionsToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
