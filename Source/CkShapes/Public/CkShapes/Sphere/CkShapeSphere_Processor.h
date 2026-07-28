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
            TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
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

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeSphere_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_ShapeSphere_CancelPendingRequests,
        FCk_Handle_ShapeSphere,
        ck::TReadOnly<FFragment_ShapeSphere_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------