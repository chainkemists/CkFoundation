#include "CkEntityCollection_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityCollection/CkEntityCollection_Log.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

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
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InComp, InRequestVariant);
            }), ck::policy::DontResetContainer{});
        });

        const auto UpdatedContent = UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(InHandle);

        if (PreviousContent == UpdatedContent)
        { return; }

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

        InHandle.Remove<MarkedDirtyBy>();
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
        _TransientEntity.Clear<FFragment_EntityCollections_RecordOfEntities_Previous>();
    }

    auto
        FProcessor_EntityCollection_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollections_RecordOfEntities_Previous&,
            const FFragment_EntityCollections_RecordOfEntities&)
        -> void
    {
        using Previous_CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Previous_Utils;
        using CollectionRecordOfEntitiesUtilsType = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils;

        const auto& PreviousContent = Previous_CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle);
        const auto& CurrentContent = CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle);

        UUtils_Signal_EntityCollection_OnCollectionUpdated::Broadcast
        (
            InHandle,
            MakePayload(InHandle, PreviousContent, CurrentContent)
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

        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_EntityCollection_Rep>(
            LifetimeOwner, [&](UCk_Fragment_EntityCollection_Rep* InRepComp)
        {
            InRepComp->Broadcast_AddOrUpdate(UCk_Utils_EntityCollection_UE::Get_EntitiesInCollection(InHandle));
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
