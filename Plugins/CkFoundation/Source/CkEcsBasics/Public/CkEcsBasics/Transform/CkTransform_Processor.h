#pragma once

#include "CkTransform_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSBASICS_API FCk_Processor_Transform_HandleRequests
        : public TProcessor<FCk_Processor_Transform_HandleRequests, FCk_Fragment_Transform_Current, FCk_Fragment_Transform_Requests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_Transform_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            FCk_Fragment_Transform_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetLocation& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddLocationOffset& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_SetRotation& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InHandle,
            FCk_Fragment_Transform_Current& InComp,
            const FCk_Request_Transform_AddRotationOffset& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
