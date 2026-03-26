#include "CkEntityCollection_Fragment.h"

#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for EntityCollection

static struct FEntityCollectionRepHandlerRegistrar
{
    FEntityCollectionRepHandlerRegistrar()
    {
        const auto DoApplyEntityCollections = [](FCk_Handle& Entity, const TArray<FCk_EntityCollection_Content>& NewCollections, const TArray<FCk_EntityCollection_Content>& OldCollections)
        {
            // Validate all new collection entries have valid entity references
            for (auto Index = OldCollections.Num(); Index < NewCollections.Num(); ++Index)
            {
                const auto& EntityCollectionToReplicate = NewCollections[Index];

                if (const auto& EntityCollectionEntity = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(
                        Entity, EntityCollectionToReplicate.Get_CollectionName());
                    ck::Is_NOT_Valid(EntityCollectionEntity))
                { return; }

                const auto AllValidEntities = ck::algo::AllOf(EntityCollectionToReplicate.Get_EntitiesInCollection(), [](
                    const FCk_Handle& MaybeValidHandle)
                {
                    return ck::IsValid(MaybeValidHandle);
                });

                if (NOT AllValidEntities)
                { return; }
            }

            // Delegate to the SyncReplication processor to handle the actual logic
            Entity.AddOrGet<ck::FFragment_EntityCollection_SyncReplication>(NewCollections, OldCollections);
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_EntityCollections::StaticStruct(); },
            {
                .OnChange = [DoApplyEntityCollections](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    DoApplyEntityCollections(Entity, New.Get<FCk_RepData_EntityCollections>().EntityCollections, Old.Get<FCk_RepData_EntityCollections>().EntityCollections);
                },
                .OnAdd = [DoApplyEntityCollections](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyEntityCollections(Entity, Data.Get<FCk_RepData_EntityCollections>().EntityCollections, TArray<FCk_EntityCollection_Content>{});
                }
            });
    }
} GEntityCollectionRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
