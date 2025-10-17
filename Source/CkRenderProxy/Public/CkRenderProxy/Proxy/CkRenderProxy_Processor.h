#pragma once

#include "CkRenderProxy/Proxy/CkRenderProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

    class CKRENDERPROXY_API FProcessor_RenderProxy_Setup : public ck_exp::TProcessor<
        FProcessor_RenderProxy_Setup,
        FCk_Handle_RenderProxy,
        FFragment_RenderProxy_Params,
        FFragment_RenderProxy_Current,
        FFragment_Transform,
        FTag_RenderProxy_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_RenderProxy_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RenderProxy_Params& InParams,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRENDERPROXY_API FProcessor_RenderProxy_UpdateTransform : public ck_exp::TProcessor<
        FProcessor_RenderProxy_UpdateTransform,
        FCk_Handle_RenderProxy,
        FFragment_RenderProxy_Current,
        FFragment_Transform,
        FTag_RenderProxy_Movable,
        FTag_Transform_Updated,
        TExclude<FTag_RenderProxy_Disabled>,
        TExclude<FTag_RenderProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;

    private:
        TSet<UWorld*> _WorldsToMarkDirty;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRENDERPROXY_API FProcessor_RenderProxy_EnsureStaticNotMoved_DEBUG : public ck_exp::TProcessor<
        FProcessor_RenderProxy_EnsureStaticNotMoved_DEBUG,
        FCk_Handle_RenderProxy,
        FFragment_RenderProxy_Params,
        TExclude<FTag_RenderProxy_Movable>,
        FTag_Transform_Updated,
        TExclude<FTag_RenderProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RenderProxy_Params& InParams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRENDERPROXY_API FProcessor_RenderProxy_HandleRequests : public ck_exp::TProcessor<
        FProcessor_RenderProxy_HandleRequests,
        FCk_Handle_RenderProxy,
        FFragment_RenderProxy_Current,
        FFragment_RenderProxy_Requests,
        TExclude<FTag_RenderProxy_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_RenderProxy_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_RenderProxy_Requests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FCk_Request_RenderProxy_EnableDisable& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRENDERPROXY_API FProcessor_RenderProxy_EndPlay : public ck_exp::TProcessor<
        FProcessor_RenderProxy_EndPlay,
        FCk_Handle_RenderProxy,
        FFragment_RenderProxy_Current,
        CK_IF_END_PLAY>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------