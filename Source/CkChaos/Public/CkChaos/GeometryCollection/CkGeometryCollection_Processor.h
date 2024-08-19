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
}

// --------------------------------------------------------------------------------------------------------------------
