#pragma once

#include "CkShapeCylinder_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeCylinder_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeCylinder_HandleRequests,
            FCk_Handle_ShapeCylinder,
            FFragment_ShapeCylinder_Params,
            FFragment_ShapeCylinder_Current,
            FFragment_ShapeCylinder_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeCylinder_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            FFragment_ShapeCylinder_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeCylinder_Params& InParams,
            FFragment_ShapeCylinder_Current& InCurrent,
            const FCk_Request_ShapeCylinder_UpdateDimensions& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------