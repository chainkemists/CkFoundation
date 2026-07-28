#pragma once

#include "CkShapeCapsule_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSHAPES_API FProcessor_ShapeCapsule_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeCapsule_HandleRequests,
            FCk_Handle_ShapeCapsule,
            ck::TReadOnly<FFragment_ShapeCapsule_Params>,
            ck::TReadWrite<FFragment_ShapeCapsule_Current>,
            ck::TReadWrite<FFragment_ShapeCapsule_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
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

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeCapsule_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_ShapeCapsule_CancelPendingRequests,
        FCk_Handle_ShapeCapsule,
        ck::TReadOnly<FFragment_ShapeCapsule_Requests>,
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
            const FFragment_ShapeCapsule_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------