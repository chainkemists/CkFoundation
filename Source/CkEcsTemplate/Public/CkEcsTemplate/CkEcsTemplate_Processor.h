#pragma once

#include "CkEcsTemplate_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSTEMPLATE_API FProcessor_EcsTemplate_Setup : public ck_exp::TProcessor<
            FProcessor_EcsTemplate_Setup,
            FCk_Handle_EcsTemplate,
            FFragment_EcsTemplate_Params,
            FFragment_EcsTemplate_Current,
            FTag_EcsTemplate_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EcsTemplate_RequiresSetup;

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
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSTEMPLATE_API FProcessor_EcsTemplate_HandleRequests : public ck_exp::TProcessor<
            FProcessor_EcsTemplate_HandleRequests,
            FCk_Handle_EcsTemplate,
            FFragment_EcsTemplate_Params,
            FFragment_EcsTemplate_Current,
            FFragment_EcsTemplate_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EcsTemplate_Requests;

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
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent,
            FFragment_EcsTemplate_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent,
            const FCk_Request_EcsTemplate_ExampleRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSTEMPLATE_API FProcessor_EcsTemplate_EndPlay : public ck_exp::TProcessor<
            FProcessor_EcsTemplate_EndPlay,
            FCk_Handle_EcsTemplate,
            FFragment_EcsTemplate_Params,
            FFragment_EcsTemplate_Current,
            CK_IF_END_PLAY>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EcsTemplate_Params& InParams,
            FFragment_EcsTemplate_Current& InCurrent) const -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------