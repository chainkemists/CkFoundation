#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKISMRENDERER_API FProcessor_IsmProxy_Setup : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Setup,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        FTag_IsmProxy_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_IsmProxy_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_AddInstance : public ck_exp::TProcessor<
        FProcessor_IsmProxy_AddInstance,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        FFragment_Transform,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_Disabled>,
        FTag_IsmProxy_NeedsInstanceAdded,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_IsmProxy_NeedsInstanceAdded;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InCurrentTransform) const -> void;

    private:
        // Refreshed every frame
        TObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_TransformInstance : public ck_exp::TProcessor<
        FProcessor_IsmProxy_TransformInstance,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        FFragment_Transform,
        TExclude<FTag_IsmProxy_Disabled>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        FTag_IsmProxy_Movable,
        FTag_Transform_Updated,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;

    private:
        // Refreshed every frame
        TObjectPtr<UWorld> _World;
        TSet<UInstancedStaticMeshComponent*> _Isms;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG : public ck_exp::TProcessor<
        FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        TExclude<FTag_IsmProxy_Disabled>,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        TExclude<FTag_IsmProxy_NeedsInstanceAdded>,
        TExclude<FTag_IsmProxy_Movable>,
        FTag_Transform_Updated,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_Teardown : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Teardown,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const -> void;

    private:
        // Refreshed every frame
        TObjectPtr<UWorld> _World;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKISMRENDERER_API FProcessor_IsmProxy_HandleRequests : public ck_exp::TProcessor<
        FProcessor_IsmProxy_HandleRequests,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        FFragment_IsmProxy_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_IsmProxy_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;
    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_IsmProxy_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomData& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomDataValue& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_EnableDisable& InRequest) const -> void;

    private:
        // Refreshed every frame
        TObjectPtr<UWorld> _World;
    };
}

// --------------------------------------------------------------------------------------------------------------------
