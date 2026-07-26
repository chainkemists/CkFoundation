#include "CkEntityCollection_Fragment.h"

#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

static struct FEntityCollectionRepHandlerRegistrar
{
    FEntityCollectionRepHandlerRegistrar()
    {
        const auto DoApplyEntityCollections = [](FCk_Handle& Entity, const TArray<FCk_EntityCollection_Content>& NewCollections, const TArray<FCk_EntityCollection_Content>& OldCollections)
        {
            // Never gate here — FProcessor_EntityCollection_SyncReplication owns the gates and the per-tick
            // retry (Claude.md § Persistence and replication).
            Entity.AddOrGet<ck::FFragment_EntityCollection_SyncReplication>(NewCollections, OldCollections);
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_EntityCollections>({
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_EntityCollection_UE::Has_Any(Entity))
                    { return {}; }

                    auto Data = FCk_RepData_EntityCollections{};
                    UCk_Utils_EntityCollection_UE::ForEach_EntityCollection(Entity,
                        [&](FCk_Handle_EntityCollection InCollection) -> void
                        {
                            Data.EntityCollections.Emplace(UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(InCollection));
                        });
                    return FInstancedStruct::Make(Data);
                },
                .NetApply = [DoApplyEntityCollections](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    DoApplyEntityCollections(Entity,
                        New.Get<FCk_RepData_EntityCollections>().EntityCollections,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_EntityCollections>().EntityCollections
                            : TArray<FCk_EntityCollection_Content>{});
                    return ECk_Persistence_ApplyResult::Applied;
                },
                // Every `return NotReady` PRECEDES the first mutation (Claude.md § Persistence and replication).
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_EntityCollection_UE::Has_Any(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& SavedCollections = New.Get<FCk_RepData_EntityCollections>().EntityCollections;

                    for (const auto& SavedCollection : SavedCollections)
                    {
                        const auto Child = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(Entity, SavedCollection.Get_CollectionName());
                        if (ck::Is_NOT_Valid(Child))
                        { return ECk_Persistence_ApplyResult::NotReady; }

                        if (ck::algo::AnyOf(SavedCollection.Get_EntitiesInCollection(), [](const FCk_Handle& InMember)
                            { return ck::Is_NOT_Valid(InMember); }))
                        { return ECk_Persistence_ApplyResult::NotReady; }
                    }

                    // ADD-only, deliberately — REMOVE-then-ADD would stack construct-seeded members that are
                    // still enqueued and invisible to a Get_ read (Claude.md § Persistence and replication).
                    for (const auto& SavedCollection : SavedCollections)
                    {
                        auto Child = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(Entity, SavedCollection.Get_CollectionName());
                        UCk_Utils_EntityCollection_UE::Request_AddEntities(Child,
                            FCk_Request_EntityCollection_AddEntities{SavedCollection.Get_EntitiesInCollection()});
                    }

                    return ECk_Persistence_ApplyResult::Applied;
                }});
    }
} GEntityCollectionRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
