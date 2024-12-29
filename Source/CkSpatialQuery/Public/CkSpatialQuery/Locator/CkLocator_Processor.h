#pragma once

#include "CkLocator_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class CKSPATIALQUERY_API FProcessor_Locator_Setup : public ck_exp::TProcessor<
            FProcessor_Locator_Setup,
            FCk_Handle_Locator,
            FFragment_Locator_Params,
            FFragment_Locator_Current,
            FTag_Locator_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Locator_RequiresSetup;

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
            const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Locator_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Locator_HandleRequests,
            FCk_Handle_Locator,
            FFragment_Locator_Params,
            FFragment_Locator_Current,
            FFragment_Locator_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Locator_Requests;

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
            const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent,
            FFragment_Locator_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent,
            const FCk_Request_Locator_ExampleRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

	class CKSPATIALQUERY_API FProcessor_Locator_Teardown : public ck_exp::TProcessor<
            FProcessor_Locator_Teardown,
            FCk_Handle_Locator,
            FFragment_Locator_Params,
            FFragment_Locator_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Locator_Params& InParams,
            FFragment_Locator_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Locator_Replicate : public ck_exp::TProcessor<
            FProcessor_Locator_Replicate,
            FCk_Handle_Locator,
            FFragment_Locator_Current,
            TObjectPtr<UCk_Fragment_Locator_Rep>,
            FTag_Locator_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Locator_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Locator_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Locator_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------