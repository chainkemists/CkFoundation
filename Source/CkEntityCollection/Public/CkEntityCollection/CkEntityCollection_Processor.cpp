#include "CkEntityCollection_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEntityCollection/CkEntityCollection_Log.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"

#include "CkNet/CkNet_Utils.h"

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
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_EntityCollection_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InComp, InRequestVariant);
            }), ck::policy::DontResetContainer{});
        });
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
            CollectionRecordOfEntitiesUtilsType::Request_Connect(InHandle, EntityToAdd);
        }

        UCk_Utils_EntityCollection_UE::Request_CollectionUpdated(InHandle);
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

        UCk_Utils_EntityCollection_UE::Request_CollectionUpdated(InHandle);
    }

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

        UUtils_Signal_EntityCollection_OnCollectionUpdated::Broadcast
        (
            InHandle,
            MakePayload(InHandle, Previous_CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle), CollectionRecordOfEntitiesUtilsType::Get_Entries(InHandle))
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
