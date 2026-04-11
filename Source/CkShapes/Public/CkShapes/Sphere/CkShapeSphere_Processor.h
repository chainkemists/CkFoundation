#pragma once

#include "CkShapeSphere_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeSphere_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeSphere_HandleRequests,
            FCk_Handle_ShapeSphere,
            ck::TReadOnly<FFragment_ShapeSphere_Params>,
            ck::TReadWrite<FFragment_ShapeSphere_Current>,
            ck::TReadWrite<FFragment_ShapeSphere_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FFragment_ShapeSphere_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            FFragment_ShapeSphere_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeSphere_Params& InParams,
            FFragment_ShapeSphere_Current& InCurrent,
            const FCk_Request_ShapeSphere_UpdateDimensions& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------