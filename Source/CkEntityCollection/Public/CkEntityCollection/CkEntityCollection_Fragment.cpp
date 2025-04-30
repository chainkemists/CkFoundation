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
    auto AssociatedEntity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(AssociatedEntity))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    AssociatedEntity.AddOrGet<ck::FFragment_EntityCollection_SyncReplication>(_EntityCollectionsToReplicate, _EntityCollectionsToReplicate_Previous);

    _EntityCollectionsToReplicate_Previous = _EntityCollectionsToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
