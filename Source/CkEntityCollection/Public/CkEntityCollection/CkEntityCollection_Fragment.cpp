#include "CkEntityCollection_Fragment.h"

#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"
#include "UObject/Class.h" // UScriptStruct::SerializeItem

// --------------------------------------------------------------------------------------------------------------------

using FSnap_RecordOfEntityCollections = ck::FFragment_RecordOfEntityCollections;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfEntityCollections);

using FSnap_EntityCollections_RecordOfEntities = ck::FFragment_EntityCollections_RecordOfEntities;
CK_REGISTER_SNAPSHOTABLE(FSnap_EntityCollections_RecordOfEntities);

using FSnap_EntityCollection_Params = ck::FFragment_EntityCollection_Params;
CK_REGISTER_SNAPSHOTABLE(FSnap_EntityCollection_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_EntityCollection_Params::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    FCk_Fragment_EntityCollection_ParamsData::StaticStruct()->SerializeItem(InAr, &_Params, nullptr);
}

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

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_EntityCollections::StaticStruct(); },
            {
                .Apply = [DoApplyEntityCollections](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    DoApplyEntityCollections(Entity,
                        New.Get<FCk_RepData_EntityCollections>().EntityCollections,
                        Old.IsSet()
                            ? Old.GetValue().Get<FCk_RepData_EntityCollections>().EntityCollections
                            : TArray<FCk_EntityCollection_Content>{});
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GEntityCollectionRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
