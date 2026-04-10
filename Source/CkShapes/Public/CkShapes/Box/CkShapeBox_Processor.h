#pragma once

#include "CkShapeBox_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeBox_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeBox_HandleRequests,
            FCk_Handle_ShapeBox,
            ck::TReadOnly<FFragment_ShapeBox_Params>,
            ck::TReadWrite<FFragment_ShapeBox_Current>,
            ck::TReadWrite<FFragment_ShapeBox_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_ShapeBox_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            FFragment_ShapeBox_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            const FCk_Request_ShapeBox_UpdateDimensions& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------