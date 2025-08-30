#pragma once

#include "CkObjectiveOwner_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOBJECTIVE_API FProcessor_ObjectiveOwner_Setup : public ck_exp::TProcessor<
            FProcessor_ObjectiveOwner_Setup,
            FCk_Handle_ObjectiveOwner,
            FFragment_ObjectiveOwner_Params,
            FTag_ObjectiveOwner_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_ObjectiveOwner_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Params& InParams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOBJECTIVE_API FProcessor_ObjectiveOwner_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ObjectiveOwner_HandleRequests,
            FCk_Handle_ObjectiveOwner,
            FFragment_ObjectiveOwner_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_ObjectiveOwner_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ObjectiveOwner_AddObjective& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------