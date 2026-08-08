#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputBias_Fragment.h"
#include "CkInput/CkInputSource_Fragment.h"
#include "CkInput/CkInput_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Retunes drain in the collect group, one group ahead of the conditioning pass, so a conditioning pass always
    // runs against a table that cannot change underneath it. A retune enqueued from gameplay — which runs after
    // routing — therefore first conditions the NEXT event to arrive, never the sample already recorded.
    class CKINPUT_API FProcessor_InputBias_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InputBias_HandleRequests,
            FCk_Handle_InputBias,
            ck::TReadWrite<FFragment_InputBias_Params>,
            ck::TReadWrite<FFragment_InputBias_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Collect;
        using MarkedDirtyBy = FFragment_InputBias_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputBias,
            FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InInputBias,
            FFragment_InputBias_Params& InParams,
            const FCk_Request_InputBias_SetAxisBias& InRequest) -> ECk_Request_OperationResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed bias's still-queued requests are
    // never drained. This fires each pending request's completion delegate with Failed_Cancelled so a caller
    // awaiting completion terminates instead of hanging.
    class CKINPUT_API FProcessor_InputBias_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_InputBias_CancelPendingRequests,
            FCk_Handle_InputBias,
            ck::TReadOnly<FFragment_InputBias_Requests>,
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
            HandleType InInputBias,
            const FFragment_InputBias_Requests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Reads the source's inbox WITHOUT draining it — the router owns the drain — and refreshes the conditioned value
    // of every axis row it finds. Buttons are not conditioned at all: a press has no magnitude for a deadzone or a
    // curve to act on.
    class CKINPUT_API FProcessor_InputBias_Condition : public ck_exp::TProcessor<
            FProcessor_InputBias_Condition,
            FCk_Handle_InputBias,
            ck::TReadOnly<FFragment_InputBias_Params>,
            ck::TReadWrite<FFragment_InputBias_Current>,
            ck::TReadOnly<FFragment_InputSource_Current>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Bias;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputBias,
            const FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Current& InCurrent,
            const FFragment_InputSource_Current& InSourceCurrent) const -> void;

    private:
        static auto
        DoConditionAxis(
            HandleType InInputBias,
            const FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Current& InCurrent,
            const FCk_InputSource_RawEvent& InEvent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
