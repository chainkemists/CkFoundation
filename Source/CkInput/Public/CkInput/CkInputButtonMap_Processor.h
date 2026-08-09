#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputButtonMap_Fragment.h"
#include "CkInput/CkInputSource_Fragment.h"
#include "CkInput/CkInput_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Derivation drains in the collect group, alongside every other CkInput request and ahead of routing, so a
    // consumer woken by a delivered event resolves buttons against this frame's map rather than last frame's.
    // A re-derive enqueued from gameplay — which runs after routing — therefore first shows up on the NEXT
    // frame, the same one-frame boundary capture edits and axis retunes obey.
    //
    // The source params ride the view because the map is only ever composed onto an input source, and the
    // player index on them is what resolves the profile the mapped tier derives from.
    class CKINPUT_API FProcessor_InputButtonMap_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InputButtonMap_HandleRequests,
            FCk_Handle_InputButtonMap,
            ck::TReadOnly<FFragment_InputSource_Params>,
            ck::TReadWrite<FFragment_InputButtonMap_Current>,
            ck::TReadWrite<FFragment_InputButtonMap_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Collect;
        using MarkedDirtyBy = FFragment_InputButtonMap_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            FFragment_InputButtonMap_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            const FCk_Request_InputButtonMap_Rederive& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InButtonMap,
            const FFragment_InputSource_Params& InSourceParams,
            FFragment_InputButtonMap_Current& InCurrent,
            const FCk_Request_InputButtonMap_RegisterPhysicalButton& InRequest) -> ECk_Request_OperationResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed map's still-queued
    // derivations are never drained. This completes each of them Failed_Cancelled instead of leaving a caller
    // waiting on a request nothing will ever look at.
    class CKINPUT_API FProcessor_InputButtonMap_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_InputButtonMap_CancelPendingRequests,
            FCk_Handle_InputButtonMap,
            ck::TReadOnly<FFragment_InputButtonMap_Requests>,
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
            HandleType InButtonMap,
            const FFragment_InputButtonMap_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
