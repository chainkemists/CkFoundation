#pragma once

#include "CkGeometryCollection_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCHAOS_API FProcessor_GeometryCollection_HandleRequests : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_HandleRequests,
            FCk_Handle_GeometryCollection,
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
            const FCk_Request_GeometryCollection_ApplyRadialStrain& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollection_CrumbleNonActiveClusters : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_CrumbleNonActiveClusters,
            FCk_Handle_GeometryCollection,
            FFragment_GeometryCollection_Params,
            FTag_GeometryCollection_CrumbleNonAnchoredClusters,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCHAOS_API FProcessor_GeometryCollection_RemoveAllAnchors : public ck_exp::TProcessor<
            FProcessor_GeometryCollection_RemoveAllAnchors,
            FCk_Handle_GeometryCollection,
            FFragment_GeometryCollection_Params,
            FTag_GeometryCollection_RemoveAllAnchors,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_GeometryCollection_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_GeometryCollection_Params& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
