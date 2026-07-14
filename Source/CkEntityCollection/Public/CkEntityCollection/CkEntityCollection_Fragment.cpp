#include "CkEntityCollection_Fragment.h"

#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h" // ck::algo::AnyOf — HydrationApply member-validity gate

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for EntityCollection

static struct FEntityCollectionRepHandlerRegistrar
{
    FEntityCollectionRepHandlerRegistrar()
    {
        const auto DoApplyEntityCollections = [](FCk_Handle& Entity, const TArray<FCk_EntityCollection_Content>& NewCollections, const TArray<FCk_EntityCollection_Content>& OldCollections)
        {
            // Always stash the snapshot. The SyncReplication processor has its own (stronger)
            // validity check on FFragment_EntityCollection_SyncReplication every tick — gating on
            // local child collection existence + per-entity EntityReplicationDriver completeness —
            // and only removes the fragment on successful apply. That gives us automatic per-tick
            // retry, which the previous rep-handler-side early-return defeated: snapshot was
            // dropped at first delivery and never re-sent (container reps only fire OnChange when
            // the snapshot changes), so a transient NetGuid-pending state at first-rep-arrival
            // permanently lost the payload. Symptom pinned by the AS net tests
            // Ck.EntityCollection.Net.AS_AddEntities_Replicates / AS_RemoveEntities_Replicates.
            Entity.AddOrGet<ck::FFragment_EntityCollection_SyncReplication>(NewCollections, OldCollections);
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_EntityCollections>(
                // Save capture: mirror FProcessor_EntityCollection_Replicate's live-state build. That processor runs
                // per CHILD collection and writes each child's Get_EntitiesInCollection into the OWNER's shared
                // FCk_RepData_EntityCollections container (find-or-emplace by CollectionName). Produce runs once on the
                // OWNER and rebuilds the full array by enumerating the owner's collections — field-for-field identical
                // per entry (CollectionName + member handles); array order is not load-bearing (hydration looks up by
                // name). return {} (UNSET) on any entity that hosts no collections (Has_Any false — the child
                // collection entities themselves, and every non-owner).
                [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
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
                [DoApplyEntityCollections](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    DoApplyEntityCollections(Entity,
                        New.Get<FCk_RepData_EntityCollections>().EntityCollections,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_EntityCollections>().EntityCollections
                            : TArray<FCk_EntityCollection_Content>{});
                    return ECk_Persistence_ApplyResult::Applied;
                },
                // Save-load hydration (authority-side): the net Apply above only stamps the sync fragment that the
                // ClientOnly FProcessor_EntityCollection_SyncReplication drains, so on the loading AUTHORITY it would
                // never restore membership. Re-drive the owner-hosted collections directly instead (mirrors TagSet's
                // distinct-HydrationApply reasoning).
                //
                // Keyed on the collection OWNER — the same entity the container (FCk_RepData_EntityCollections) and
                // the net Apply are keyed on. The owner's replayed Construct re-composed each named child collection
                // EMPTY on reload (RecordOfEntityCollections + the child entities); this only re-drives membership.
                //
                // Every `return NotReady` PRECEDES the first mutation (a retry after a partial write would stack): the
                // collections feature must be composed, AND every named child re-composed, AND every saved member
                // handle re-resolved (rebuilt references settle over load-kernel ticks). All-or-nothing — mirrors the
                // ClientOnly SyncReplication processor's two gates (child TryGet + member validity).
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
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

                    // Every gate passed — re-drive. The child was re-composed EMPTY by Construct, so ADD the saved
                    // members. Request_AddEntities filters already-present members (idempotent under double-apply) and,
                    // via FProcessor_EntityCollection_HandleRequests -> Request_TryReplicateEntityCollection, re-arms
                    // FTag_EntityCollection_MayRequireReplication so the authority Replicate pass re-pushes the
                    // owner-hosted container and post-load clients converge (implicit re-arm).
                    //
                    // ADD-only, deliberately: we do NOT read the live set to REMOVE-then-ADD. At HydrationApply time
                    // any construct-seeded member Requests are still enqueued and INVISIBLE to a Get_ read (feature
                    // request-drain is GatedDuringLoad), so a read-then-clear would stack seeds under the saved set
                    // (the CkSnapshot/Claude.md §5.3 merge hazard). EntityCollection has no drain-time restore
                    // primitive; ADD-only is correct for the wire/save shape (collection composed EMPTY by Construct +
                    // members added at runtime). A game that seeds collection MEMBERS in Construct would need a
                    // composite Request_RestoreSet to REPLACE at drain time — out of scope here.
                    for (const auto& SavedCollection : SavedCollections)
                    {
                        auto Child = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(Entity, SavedCollection.Get_CollectionName());
                        UCk_Utils_EntityCollection_UE::Request_AddEntities(Child,
                            FCk_Request_EntityCollection_AddEntities{SavedCollection.Get_EntitiesInCollection()});
                    }

                    return ECk_Persistence_ApplyResult::Applied;
                });
    }
} GEntityCollectionRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
