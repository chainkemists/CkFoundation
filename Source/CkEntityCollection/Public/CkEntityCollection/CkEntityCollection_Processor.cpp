#include "CkEntityCollection_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityCollection/CkEntityCollection_Log.h"
#include "CkEntityCollection/CkEntityCollection_Stats.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("EntityCollection::StorePrevious"), STAT_EntityCollection_StorePrevious, STATGROUP_CkEntityCollection);
DECLARE_CYCLE_STAT(TEXT("EntityCollection::DiffContent"), STAT_EntityCollection_DiffContent, STATGROUP_CkEntityCollection);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntityCollection_StorePrevious);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityCollection_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityCollection_SyncReplication);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityCollection_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_EntityCollection_Replicate);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityCollection_StorePrevious::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityCollections_RecordOfEntities&,
            FFragment_EntityCollection_Requests&)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_EntityCollection_StorePrevious);

        UCk_Utils_EntityCollection_UE::Request_StorePreviousCollection(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityCollection_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            FFragment_EntityCollection_Requests& InRequestsComp) const
        -> void
    {
        const auto PreviousContent = UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(InHandle);

        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_EntityCollection_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InComp, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }), ck::policy::DontResetContainer{});
        });

        {
            SCOPE_CYCLE_COUNTER(STAT_EntityCollection_DiffContent);

            const auto UpdatedContent = UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(InHandle);

            if (PreviousContent == UpdatedContent)
            { return; }
        }

        UCk_Utils_EntityCollection_UE::Request_CollectionUpdated(InHandle);
        UCk_Utils_EntityCollection_UE::Request_TryReplicateEntityCollection(InHandle);
    }

    auto
        FProcessor_EntityCollection_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            const FFragment_EntityCollection_Requests::AddEntitiesRequestType& InRequest)
        -> void
    {
        using CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils;

        const auto& EntitiesToAdd = ck::algo::Filter(InRequest.Get_EntitiesToAdd(), [&](const FCk_Handle& InEntity)
        {
            if (ck::Is_NOT_Valid(InEntity))
            { return false; }

            if (CollectionRecordOfEntitiesUtilsType::Get_ContainsEntry(InHandle, InEntity))
            { return false; }

            return true;
        });

        if (EntitiesToAdd.IsEmpty())
        { return; }

        for (auto EntityToAdd : EntitiesToAdd)
        {
            CollectionRecordOfEntitiesUtilsType::Request_Connect(InHandle, EntityToAdd, ECk_Record_LabelRequirementPolicy::Optional);
        }
    }

    auto
        FProcessor_EntityCollection_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            const FFragment_EntityCollection_Requests::RemoveEntitiesRequestType& InRequest)
        -> void
    {
        using CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils;

        const auto& EntitiesToRemove = ck::algo::Filter(InRequest.Get_EntitiesToRemove(), [&](const FCk_Handle& InEntity)
        {
            if (ck::Is_NOT_Valid(InEntity))
            { return false; }

            if (NOT CollectionRecordOfEntitiesUtilsType::Get_ContainsEntry(InHandle, InEntity))
            { return false; }

            return true;
        });

        if (EntitiesToRemove.IsEmpty())
        { return; }

        for (auto EntityToRemove : EntitiesToRemove)
        {
            CollectionRecordOfEntitiesUtilsType::Request_Disconnect(InHandle, EntityToRemove);
        }
    }

    auto
        FProcessor_EntityCollection_SyncReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollection_SyncReplication& InSync)
        -> void
    {
        const auto& EntityCollectionsToReplicate = InSync.Get_EntityCollectionsToReplicate();
        const auto& EntityCollectionsToReplicate_Previous = InSync.Get_EntityCollectionsToReplicate_Previous();

        for (const auto& Content : EntityCollectionsToReplicate)
        {
            // Gate 1: local child collection entity must exist (entity-script Construct has run).
            // Previously implicit — the rep handler's early-return on TryGet failure protected
            // the downstream Request_RemoveEntities/AddEntities calls below from running on an
            // invalid handle. After deleting the rep-handler-side early-return (so the snapshot
            // can survive a transient pending-Setup state until retry), this gate has to move
            // here. The fragment stays on the entity until both gates pass.
            if (const auto& LocalCollection = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(InHandle, Content.Get_CollectionName());
                ck::Is_NOT_Valid(LocalCollection))
            { return; }

            // Gate 2: every entity in the snapshot must have a complete EntityReplicationDriver
            // (NetGuid handshake done). Container reps deliver the snapshot once; this per-tick
            // retry waits for the referenced entities' replication to land. Without this gate,
            // we'd local-Add entities whose handles are still in pending-resolve state, ending
            // up with broken records that never recover.
            const auto& AllValidEntities = ck::algo::AllOf(Content.Get_EntitiesInCollection(), [](
                const FCk_Handle& MaybeValidHandle)
            {
                if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(MaybeValidHandle))
                { return false; }

                if (UCk_Utils_EntityReplicationDriver_UE::Get_IsReplicationComplete(MaybeValidHandle))
                { return true; }

                if (UCk_Utils_EntityReplicationDriver_UE::Get_IsReplicationCompleteAllDependents(MaybeValidHandle))
                { return true; }

                return false;
            });

            if (NOT AllValidEntities)
            { return; }
        }

        for (auto Index = 0; Index < EntityCollectionsToReplicate.Num(); ++Index)
        {
            const auto& EntityCollectionToReplicate = EntityCollectionsToReplicate[Index];
            auto EntityCollectionEntity = UCk_Utils_EntityCollection_UE::TryGet_EntityCollection(InHandle, EntityCollectionToReplicate.Get_CollectionName());
            const auto& CurrentCollectionContent = UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(EntityCollectionEntity);

            if (NOT EntityCollectionsToReplicate_Previous.IsValidIndex(Index))
            {
                entity_collection::Verbose(TEXT("Replicating EntityCollection for the FIRST time to [{}]"), EntityCollectionToReplicate);

                UCk_Utils_EntityCollection_UE::Request_RemoveEntities(EntityCollectionEntity, FCk_Request_EntityCollection_RemoveEntities{CurrentCollectionContent.Get_EntitiesInCollection()});
                UCk_Utils_EntityCollection_UE::Request_AddEntities(EntityCollectionEntity, FCk_Request_EntityCollection_AddEntities{EntityCollectionToReplicate.Get_EntitiesInCollection()});

                continue;
            }

            if (EntityCollectionsToReplicate_Previous[Index] != EntityCollectionToReplicate)
            {
                entity_collection::Verbose(TEXT("Replicating EntityCollection and UPDATING it to [{}]"), EntityCollectionToReplicate);

                UCk_Utils_EntityCollection_UE::Request_RemoveEntities(EntityCollectionEntity, FCk_Request_EntityCollection_RemoveEntities{CurrentCollectionContent.Get_EntitiesInCollection()});
                UCk_Utils_EntityCollection_UE::Request_AddEntities(EntityCollectionEntity, FCk_Request_EntityCollection_AddEntities{EntityCollectionToReplicate.Get_EntitiesInCollection()});

                continue;
            }

            entity_collection::Verbose(TEXT("IGNORING EntityCollection [{}] as there is no change between [{}] and [{}]"),
                EntityCollectionToReplicate.Get_CollectionName(),
                EntityCollectionsToReplicate_Previous[Index],
                EntityCollectionToReplicate);
        }

        // Successful apply: remove the SyncReplication fragment so this MarkedDirtyBy-less
        // processor stops re-firing on the entity until the next rep delivery puts a new fragment on it.
        InHandle.Remove<FFragment_EntityCollection_SyncReplication>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityCollection_FireSignals::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
        // Intentionally NOT clearing FFragment_EntityCollections_RecordOfEntities_Previous
        // here. The Previous fragment is a TUtils_RecordOfEntities — its
        // connected entries store per-record disconnect lambdas on their
        // FFragment_RecordEntry, fired from FProcessor_RecordEntry_Destructor
        // during their teardown, that call Get<Previous>() on this collection.
        // Bulk-clearing Previous would leave those lambdas referencing a
        // missing fragment and ensure-fail later. The fragment's lifetime now
        // follows its entries: Request_StorePreviousCollection cleanly
        // Disconnects the prior snapshot before re-Connecting the current one.
    }

    auto
        FProcessor_EntityCollection_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollection_Params& InParams,
            const FFragment_EntityCollections_RecordOfEntities_Previous&,
            const FFragment_EntityCollections_RecordOfEntities&)
        -> void
    {
        using Previous_CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Previous_Utils;
        using CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils;

        const auto& CollectionName = InParams.Get_Params().Get_Name();

        const auto PreviousContent = Previous_CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle);
        const auto CurrentContent = CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle);

        const auto EntitiesAdded = ck::algo::Except(CurrentContent, PreviousContent);
        const auto EntitiesRemoved = ck::algo::Except(PreviousContent, CurrentContent);

        const auto PreviousContentCollection = FCk_EntityCollection_Content{CollectionName, PreviousContent};
        const auto CurrentContentCollection = FCk_EntityCollection_Content{CollectionName, CurrentContent};

        UUtils_Signal_EntityCollection_OnCollectionUpdated::Broadcast
        (
            InHandle,
            MakePayload
            (
                InHandle,
                PreviousContentCollection,
                CurrentContentCollection,
                EntitiesAdded,
                EntitiesRemoved
            )
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityCollection_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityCollection_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollection_Params& InParams,
            const FFragment_EntityCollections_RecordOfEntities&)
            -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        // One projection: the registered OWNER-keyed Produce rebuilds the full EntityCollections array via the
        // same record walk. Full-replace the owner container with it — content-identical to today's per-child
        // find-or-emplace (each collection's live members read the same way).
        const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_EntityCollections>(LifetimeOwner);
        if (Produced.IsSet())
        { UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_EntityCollections>(LifetimeOwner, *Produced); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
