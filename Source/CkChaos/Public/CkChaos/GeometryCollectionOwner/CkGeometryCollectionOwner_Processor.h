#pragma once

#include "CkGeometryCollectionOwner_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_Setup : public ck_exp::TProcessor<
            FProcessor_GeometryCollectionOwner_Setup,
            FCk_Handle_GeometryCollectionOwner,
            FTag_GeometryCollectionOwner_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_GeometryCollectionOwner_RequiresSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_HandleRequests : public ck_exp::TProcessor<
        FProcessor_GeometryCollectionOwner_HandleRequests,
        FCk_Handle_GeometryCollectionOwner,
        FFragment_GeometryCollectionOwner_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollectionOwner_Requests;

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
            const FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollectionOwner_Replicate : public ck_exp::TProcessor<
        FProcessor_GeometryCollectionOwner_Replicate,
        FCk_Handle_GeometryCollectionOwner,
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
