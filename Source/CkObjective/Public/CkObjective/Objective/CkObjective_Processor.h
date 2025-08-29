#pragma once

#include "CkObjective_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOBJECTIVE_API FProcessor_Objective_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Objective_HandleRequests,
            FCk_Handle_Objective,
            FFragment_Objective_Current,
            FFragment_Objective_Params,
            FFragment_Objective_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Objective_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FFragment_Objective_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Start& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Complete& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Fail& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_UpdateProgress& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_AddProgress& InRequest) -> void;

    private:
        static auto
        DoSetStatus(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            ECk_ObjectiveStatus NewStatus) -> void;

        static auto
        DoSetProgress(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            FGameplayTag InMetaData,
            int32 NewProgress) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------