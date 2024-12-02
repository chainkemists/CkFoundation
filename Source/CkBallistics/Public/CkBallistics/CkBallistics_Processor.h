#pragma once

#include "CkBallistics_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKBALLISTICS_API FProcessor_Ballistics_Setup : public ck_exp::TProcessor<
            FProcessor_Ballistics_Setup,
            FCk_Handle_Ballistics,
            FFragment_Ballistics_Params,
            FFragment_Ballistics_Current,
            FTag_Ballistics_RequiresSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Ballistics_RequiresSetup;

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
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKBALLISTICS_API FProcessor_Ballistics_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Ballistics_HandleRequests,
            FCk_Handle_Ballistics,
            FFragment_Ballistics_Params,
            FFragment_Ballistics_Current,
            FFragment_Ballistics_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Ballistics_Requests;

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
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent,
            FFragment_Ballistics_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent,
            const FCk_Request_Ballistics_ExampleRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKBALLISTICS_API FProcessor_Ballistics_Teardown : public ck_exp::TProcessor<
            FProcessor_Ballistics_Teardown,
            FCk_Handle_Ballistics,
            FFragment_Ballistics_Params,
            FFragment_Ballistics_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Ballistics_Params& InParams,
            FFragment_Ballistics_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKBALLISTICS_API FProcessor_Ballistics_Replicate : public ck_exp::TProcessor<
            FProcessor_Ballistics_Replicate,
            FCk_Handle_Ballistics,
            FFragment_Ballistics_Current,
            TObjectPtr<UCk_Fragment_Ballistics_Rep>,
            FTag_Ballistics_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Ballistics_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Ballistics_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Ballistics_Rep>& InRepComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------