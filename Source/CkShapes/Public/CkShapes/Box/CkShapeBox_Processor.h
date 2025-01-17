#pragma once

#include "CkShapeBox_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class CKSHAPES_API FProcessor_ShapeBox_Setup : public ck_exp::TProcessor<
            FProcessor_ShapeBox_Setup,
            FCk_Handle_ShapeBox,
            FFragment_ShapeBox_Params,
            FFragment_ShapeBox_Current,
            FTag_ShapeBox_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeBox_RequiresSetup;

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
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeBox_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ShapeBox_HandleRequests,
            FCk_Handle_ShapeBox,
            FFragment_ShapeBox_Params,
            FFragment_ShapeBox_Current,
            FFragment_ShapeBox_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ShapeBox_Requests;

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
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            FFragment_ShapeBox_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent,
            const FCk_Request_ShapeBox_ExampleRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

	class CKSHAPES_API FProcessor_ShapeBox_Teardown : public ck_exp::TProcessor<
            FProcessor_ShapeBox_Teardown,
            FCk_Handle_ShapeBox,
            FFragment_ShapeBox_Params,
            FFragment_ShapeBox_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Params& InParams,
            FFragment_ShapeBox_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSHAPES_API FProcessor_ShapeBox_Replicate : public ck_exp::TProcessor<
            FProcessor_ShapeBox_Replicate,
            FCk_Handle_ShapeBox,
            FFragment_ShapeBox_Current,
            TObjectPtr<UCk_Fragment_ShapeBox_Rep>,
            FTag_ShapeBox_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ShapeBox_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ShapeBox_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_ShapeBox_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------