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
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
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

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed query entity's still-
    // queued requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKGRAPHICS_API FProcessor_RenderStatus_CancelPendingRequests : public TProcessor<
        FProcessor_RenderStatus_CancelPendingRequests,
        TReadOnly<FFragment_RenderStatus_Requests>,
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
            const FFragment_RenderStatus_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
