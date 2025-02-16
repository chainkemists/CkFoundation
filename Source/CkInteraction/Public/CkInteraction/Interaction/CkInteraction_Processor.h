#pragma once

#include "CkInteraction_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINTERACTION_API FProcessor_Interaction_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Interaction_HandleRequests,
            FCk_Handle_Interaction,
            FFragment_Interaction_Params,
            FFragment_Interaction_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Interaction_Requests;

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
            const FFragment_Interaction_Params& InParams,
            FFragment_Interaction_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams,
            const FCk_Request_Interaction_EndInteraction& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_Interaction_Teardown : public ck_exp::TProcessor<
            FProcessor_Interaction_Teardown,
            FCk_Handle_Interaction,
            FFragment_Interaction_Params,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Interaction_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------