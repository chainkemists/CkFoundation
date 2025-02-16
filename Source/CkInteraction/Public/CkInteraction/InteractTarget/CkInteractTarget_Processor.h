#pragma once

#include "CkInteractTarget_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINTERACTION_API FProcessor_InteractTarget_Setup : public ck_exp::TProcessor<
            FProcessor_InteractTarget_Setup,
            FCk_Handle_InteractTarget,
            FFragment_InteractTarget_Params,
            FFragment_InteractTarget_Current,
            FTag_InteractTarget_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_InteractTarget_RequiresSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InComp) -> void;
    };

    class CKINTERACTION_API FProcessor_InteractTarget_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InteractTarget_HandleRequests,
            FCk_Handle_InteractTarget,
            FFragment_InteractTarget_Params,
            FFragment_InteractTarget_Current,
            FFragment_InteractTarget_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_InteractTarget_Requests;

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
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InComp,
            FFragment_InteractTarget_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InCurrent,
            const FCk_Try_InteractTarget_StartInteraction& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InCurrent,
            const FCk_Request_InteractTarget_CancelInteraction& InRequest) const -> void;

    private:
        auto
        OnInteractionFinished(
            FCk_Handle_Interaction InteractionHandle,
            ECk_SucceededFailed SucceededFailed) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractTarget_Teardown : public ck_exp::TProcessor<
            FProcessor_InteractTarget_Teardown,
            FCk_Handle_InteractTarget,
            FFragment_InteractTarget_Params,
            FFragment_InteractTarget_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractTarget_Params& InParams,
            FFragment_InteractTarget_Current& InComp) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractTarget_Replicate : public ck_exp::TProcessor<
            FProcessor_InteractTarget_Replicate,
            FCk_Handle_InteractTarget,
            FFragment_InteractTarget_Current,
            TObjectPtr<UCk_Fragment_InteractTarget_Rep>,
            FTag_InteractTarget_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_InteractTarget_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_InteractTarget_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_InteractTarget_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------