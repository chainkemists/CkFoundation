#pragma once

#include "CkObjectiveOwner_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKOBJECTIVE_API FProcessor_ObjectiveOwner_Setup : public ck_exp::TProcessor<
            FProcessor_ObjectiveOwner_Setup,
            FCk_Handle_ObjectiveOwner,
            TReadOnly<FFragment_ObjectiveOwner_Params>,
            TReadWrite<FFragment_ObjectiveOwner_Current>,
            FTag_ObjectiveOwner_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_ObjectiveOwner_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ObjectiveOwner_Params& InParams,
            FFragment_ObjectiveOwner_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOBJECTIVE_API FProcessor_ObjectiveOwner_HandleRequests : public ck_exp::TProcessor<
            FProcessor_ObjectiveOwner_HandleRequests,
            FCk_Handle_ObjectiveOwner,
            TReadWrite<FFragment_ObjectiveOwner_Current>,
            TReadOnly<FFragment_ObjectiveOwner_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_ObjectiveOwner_Setup>;
        using MarkedDirtyBy = FFragment_ObjectiveOwner_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FFragment_ObjectiveOwner_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FCk_Request_ObjectiveOwner_AddObjective& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_ObjectiveOwner_Current& InCurrent,
            const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------