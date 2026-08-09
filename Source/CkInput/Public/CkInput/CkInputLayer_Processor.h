#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkInput/CkInputLayer_Fragment.h"
#include "CkInput/CkInputSource_Fragment.h"
#include "CkInput/CkInput_ProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Every input source gets router state, whether or not a layer was ever registered on it. Without this the
    // router's view would skip layer-less sources and their inboxes would grow for the life of the session.
    //
    // The per-frame retention fragment is stamped in the same pass, so the two always co-exist and the router
    // never has to test for one of them.
    class CKINPUT_API FProcessor_InputLayer_SetupRouterState : public ck_exp::TProcessor<
            FProcessor_InputLayer_SetupRouterState,
            FCk_Handle_InputSource,
            ck::TReadOnly<FFragment_InputSource_Current>,
            TExclude<FFragment_InputLayer_RouterState>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Collect;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            const FFragment_InputSource_Current& InCurrent) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Capture edits drain HERE, in the collect group, so the routing pass that follows sees a capture set that
    // cannot change underneath it. An edit enqueued during or after routing therefore first takes effect on the
    // next frame's routing — the one-frame transition contract.
    class CKINPUT_API FProcessor_InputLayer_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InputLayer_HandleRequests,
            FCk_Handle_InputLayer,
            ck::TReadWrite<FFragment_InputLayer_Current>,
            ck::TReadWrite<FFragment_InputLayer_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Collect;
        using MarkedDirtyBy = FFragment_InputLayer_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            FFragment_InputLayer_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            const FCk_Request_InputLayer_AddCapture& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InLayer,
            FFragment_InputLayer_Current& InCurrent,
            const FCk_Request_InputLayer_RemoveCapture& InRequest) -> ECk_Request_OperationResult;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKINPUT_API FProcessor_InputLayer_CancelPendingRequests : public ck_exp::TProcessor<
            FProcessor_InputLayer_CancelPendingRequests,
            FCk_Handle_InputLayer,
            ck::TReadOnly<FFragment_InputLayer_Requests>,
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
            HandleType InLayer,
            const FFragment_InputLayer_Requests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Drains the source inbox and arbitrates every row against that source's layers, highest priority first. The
    // first matching Consume capture ends the walk; a PassThrough capture is delivered and the walk continues.
    class CKINPUT_API FProcessor_InputLayer_Route : public ck_exp::TProcessor<
            FProcessor_InputLayer_Route,
            FCk_Handle_InputSource,
            ck::TReadWrite<FFragment_InputSource_Current>,
            ck::TReadWrite<FFragment_InputLayer_RouterState>,
            ck::TReadWrite<FFragment_InputLayer_RoutedThisFrame>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Input_Route;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            FFragment_InputLayer_RouterState& InRouterState,
            FFragment_InputLayer_RoutedThisFrame& InRouted) const -> void;

    private:
        static auto
        DoRankLayers(
            HandleType InInputSource,
            FFragment_InputLayer_RouterState& InRouterState) -> void;

        static auto
        DoRouteEvent(
            HandleType InInputSource,
            FFragment_InputLayer_RouterState& InRouterState,
            FFragment_InputLayer_RoutedThisFrame& InRouted,
            const FCk_InputSource_RawEvent& InEvent) -> void;

        static auto
        DoFindPressOwnerIndex(
            const FFragment_InputLayer_RouterState& InRouterState,
            const FKey& InKey) -> int32;

        static auto
        DoConsumeOwnedRelease(
            FFragment_InputLayer_RouterState& InRouterState,
            FFragment_InputLayer_RoutedThisFrame& InRouted,
            int32 InOwnerIndex,
            const FCk_InputSource_RawEvent& InEvent) -> void;

        static auto
        DoDeliver(
            FCk_Handle_InputLayer InLayer,
            const FCk_InputSource_RawEvent& InEvent,
            const FCk_InputLayer_Capture& InCapture) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
