#pragma once

#include "CkGeometryCollection_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCHAOS_API FProcessor_GeometryCollection_HandleRequests : public TProcessor<
            FProcessor_GeometryCollection_HandleRequests,
            FFragment_GeometryCollection_Params,
            FFragment_GeometryCollection_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FFragment_GeometryCollection_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyStrain& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams,
            const FCk_Request_GeometryCollection_ApplyAoE& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_Setup : public ck_exp::TProcessor<
            FProcessor_GeometryCollectionOwner_Setup,
            FCk_Handle_GeometryCollectionOwner,
            FTag_GeometryCollectionOwner_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_ReplicationRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_HandleRequests : public TProcessor<
            FProcessor_GeometryCollectionOwner_HandleRequests,
            FFragment_GeometryCollectionOwner_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollectionOwner_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GeometryCollection_ApplyStrain_Replicated& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_GeometryCollection_ApplyAoE_Replicated& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_Replicate : public TProcessor<
            FProcessor_GeometryCollectionOwner_Replicate,
            TObjectPtr<UCk_Fragment_GeometryCollectionOwner_Rep>,
            FFragment_GeometryCollection_ReplicationRequests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_ReplicationRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_GeometryCollectionOwner_Rep>& InComp,
            const FFragment_GeometryCollection_ReplicationRequests& InRequestComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
