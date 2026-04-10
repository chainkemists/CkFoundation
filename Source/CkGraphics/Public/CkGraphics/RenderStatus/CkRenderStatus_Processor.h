#pragma once

#include "CkRenderStatus_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKGRAPHICS_API FProcessor_RenderStatus_HandleRequests : public TProcessor<
            FProcessor_RenderStatus_HandleRequests,
            TReadWrite<FFragment_RenderStatus_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_RenderStatus_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderStatus_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_RenderStatus_QueryRenderedActors& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
