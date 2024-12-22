#pragma once

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKISMRENDERER_API FProcessor_IsmProxy_Static : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Static,
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

    class CKISMRENDERER_API FProcessor_IsmProxy_Dynamic : public ck_exp::TProcessor<
        FProcessor_IsmProxy_Dynamic,
        FCk_Handle_IsmProxy,
        FFragment_IsmProxy_Params,
        FFragment_IsmProxy_Current,
        TExclude<FTag_IsmProxy_NeedsSetup>,
        FTag_IsmProxy_Dynamic,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const -> void;
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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_IsmProxy_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomData& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomDataValue& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
