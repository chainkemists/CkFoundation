#pragma once

#include "CkShapeCapsule_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeCapsule_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_HandleRequests,
            FCk_Handle_ShapeCapsule,
            FFragment_ShapeCapsule_Params,
            FFragment_ShapeCapsule_Current,
            FFragment_ShapeCapsule_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeCapsule_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            FFragment_ShapeCapsule_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCapsule_Params& InParams,
            FFragment_ShapeCapsule_Current& InCurrent,
            const FCk_Request_ShapeCapsule_UpdateDimensions& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------