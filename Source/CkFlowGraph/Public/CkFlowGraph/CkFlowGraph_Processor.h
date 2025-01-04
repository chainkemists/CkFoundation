#pragma once

#include "CkFlowGraph_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKFLOWGRAPH_API FProcessor_FlowGraph_HandleRequests : public ck_exp::TProcessor<
            FProcessor_FlowGraph_HandleRequests,
            FCk_Handle_FlowGraph,
            FFragment_FlowGraph_Params,
            FFragment_FlowGraph_Current,
            FFragment_FlowGraph_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_FlowGraph_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FFragment_FlowGraph_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FCk_Request_FlowGraph_Start& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_FlowGraph_Params& InParams,
            FFragment_FlowGraph_Current& InCurrent,
            const FCk_Request_FlowGraph_Stop& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------