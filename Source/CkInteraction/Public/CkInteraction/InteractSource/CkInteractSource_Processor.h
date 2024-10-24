#pragma once

#include "CkInteractSource_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class CKINTERACTION_API FProcessor_InteractSource_Setup : public ck_exp::TProcessor<
            FProcessor_InteractSource_Setup,
            FCk_Handle_InteractSource,
            FFragment_InteractSource_Params,
            FFragment_InteractSource_Current,
            FTag_InteractSource_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_InteractSource_RequiresSetup;

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
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp) const -> void;
    };

    class CKINTERACTION_API FProcessor_InteractSource_HandleRequests : public ck_exp::TProcessor<
            FProcessor_InteractSource_HandleRequests,
            FCk_Handle_InteractSource,
            FFragment_InteractSource_Params,
            FFragment_InteractSource_Current,
            FFragment_InteractSource_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_InteractSource_Requests;

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
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp,
            FFragment_InteractSource_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InCurrent,
            const FCk_Request_InteractSource_StartInteraction& InRequest) const -> void;

    	auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InCurrent,
            const FCk_Request_InteractSource_CancelInteraction& InRequest) const -> void;

    private:
    	auto
    	OnInteractionFinished(
    		FCk_Handle_Interaction InteractionHandle,
    		ECk_SucceededFailed SucceededFailed) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

	class CKINTERACTION_API FProcessor_InteractSource_Teardown : public ck_exp::TProcessor<
            FProcessor_InteractSource_Teardown,
            FCk_Handle_InteractSource,
            FFragment_InteractSource_Params,
            FFragment_InteractSource_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_InteractSource_Params& InParams,
            FFragment_InteractSource_Current& InComp) const -> void;
    };

	// --------------------------------------------------------------------------------------------------------------------

    class CKINTERACTION_API FProcessor_InteractSource_Replicate : public ck_exp::TProcessor<
            FProcessor_InteractSource_Replicate,
            FCk_Handle_InteractSource,
            FFragment_InteractSource_Current,
            TObjectPtr<UCk_Fragment_InteractSource_Rep>,
            FTag_InteractSource_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_InteractSource_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_InteractSource_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_InteractSource_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------