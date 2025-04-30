#pragma once

#include "CkEntityCollection/CkEntityCollection_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYCOLLECTION_API FProcessor_EntityCollection_StorePrevious : public ck_exp::TProcessor<
            FProcessor_EntityCollection_StorePrevious,
            FCk_Handle_EntityCollection,
            FFragment_EntityCollections_RecordOfEntities,
            FFragment_EntityCollection_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityCollections_RecordOfEntities&,
            FFragment_EntityCollection_Requests&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKENTITYCOLLECTION_API FProcessor_EntityCollection_HandleRequests : public ck_exp::TProcessor<
            FProcessor_EntityCollection_HandleRequests,
            FCk_Handle_EntityCollection,
            FFragment_EntityCollections_RecordOfEntities,
            FFragment_EntityCollection_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            FFragment_EntityCollection_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            const FFragment_EntityCollection_Requests::AddEntitiesRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_EntityCollections_RecordOfEntities& InComp,
            const FFragment_EntityCollection_Requests::RemoveEntitiesRequestType& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKENTITYCOLLECTION_API FProcessor_EntityCollection_SyncReplication : public ck_exp::TProcessor<
        FProcessor_EntityCollection_SyncReplication,
        FCk_Handle_EntityCollection,
        FFragment_EntityCollection_SyncReplication,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EntityCollection_SyncReplication;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollection_SyncReplication& InSync) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKENTITYCOLLECTION_API FProcessor_EntityCollection_FireSignals : public ck_exp::TProcessor<
        FProcessor_EntityCollection_FireSignals,
        FCk_Handle_EntityCollection,
        FFragment_EntityCollections_RecordOfEntities_Previous,
        FFragment_EntityCollections_RecordOfEntities,
        FTag_EntityCollection_CollectionUpdated,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EntityCollection_CollectionUpdated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollections_RecordOfEntities_Previous&,
            const FFragment_EntityCollections_RecordOfEntities&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKENTITYCOLLECTION_API FProcessor_EntityCollection_Replicate : public ck_exp::TProcessor<
            FProcessor_EntityCollection_Replicate,
            FCk_Handle_EntityCollection,
            FFragment_EntityCollection_Params,
            FFragment_EntityCollections_RecordOfEntities,
            FTag_EntityCollection_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EntityCollection_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityCollection_Params& InParams,
            const FFragment_EntityCollections_RecordOfEntities&) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
