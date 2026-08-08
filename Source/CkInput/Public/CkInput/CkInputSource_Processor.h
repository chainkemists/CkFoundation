#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInput_ProcessorGroups.h"
#include "CkInput/CkInputSource_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINPUT_API FProcessor_InputSource_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InputSource_HandleRequests,
            FCk_Handle_InputSource,
            ck::TReadWrite<FFragment_InputSource_Current>,
            ck::TReadWrite<FFragment_InputSource_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Collect;
        using MarkedDirtyBy = FFragment_InputSource_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            FFragment_InputSource_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            const FCk_Request_InputSource_InjectRawEvent& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            const FCk_Request_InputSource_AssignDevice& InRequest) -> ECk_Request_OperationResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed source's still-queued
    // requests are never drained. This fires each pending request's completion delegate with Failed_Cancelled
    // so a caller awaiting completion terminates instead of hanging.
    class CKINPUT_API FProcessor_InputSource_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_InputSource_CancelPendingRequests,
            FCk_Handle_InputSource,
            ck::TReadOnly<FFragment_InputSource_Requests>,
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
            HandleType InInputSource,
            const FFragment_InputSource_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
