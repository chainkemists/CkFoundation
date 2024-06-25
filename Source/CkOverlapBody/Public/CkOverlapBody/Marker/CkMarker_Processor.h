#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOVERLAPBODY_API FProcessor_Marker_Setup : public ck_exp::TProcessor<
            FProcessor_Marker_Setup,
            FCk_Handle_Marker,
            FFragment_Marker_Current,
            FFragment_Marker_Params,
            FTag_Marker_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Marker_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Marker_HandleRequests,
            FCk_Handle_Marker,
            FFragment_Marker_Current,
            FFragment_Marker_Params,
            FFragment_Marker_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Marker_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp,
            const FFragment_Marker_Requests& InRequestsComp) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp,
            const FCk_Request_Marker_EnableDisable& InRequest) -> void;

        static auto DoHandleRequest(
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp,
            const FCk_Request_Marker_Resize& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_Teardown : public ck_exp::TProcessor<
            FProcessor_Marker_Teardown,
            FCk_Handle_Marker,
            FFragment_Marker_Current,
            FFragment_Marker_Params,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using MarkedDirtyBy = ck::FTag_DestroyEntity_Initiate;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params& InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Marker_UpdateTransform,
            FCk_Handle_Marker,
            FFragment_Marker_Current,
            FFragment_Marker_Params,
            FTag_Marker_UpdateTransform,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InMarkerEntity,
            const FFragment_Marker_Current& InCurrentComp,
            const FFragment_Marker_Params&  InParamsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOVERLAPBODY_API FProcessor_Marker_DebugPreviewAll
    {
    public:
        explicit FProcessor_Marker_DebugPreviewAll(
            const FCk_Registry& InRegistry);

    public:
        auto Tick(FCk_Time) -> void;

    private:
        FCk_Registry _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
