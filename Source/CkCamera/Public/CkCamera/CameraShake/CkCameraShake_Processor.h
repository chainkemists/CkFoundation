#pragma once

#include "CkCameraShake_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCAMERA_API FProcessor_CameraShake_HandleRequests : public TProcessor<
            FProcessor_CameraShake_HandleRequests,
            FFragment_CameraShake_Params,
            FFragment_CameraShake_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_CameraShake_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CameraShake_Params& InParams,
            FFragment_CameraShake_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayOnTarget& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayAtLocation& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
