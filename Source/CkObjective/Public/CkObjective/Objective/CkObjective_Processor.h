#pragma once

#include "CkObjective_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ObjectiveOwner_HandleRequests;

    class CKOBJECTIVE_API FProcessor_Objective_Setup : public ck_exp::TProcessor<
            FProcessor_Objective_Setup,
            FCk_Handle_Objective,
            TReadOnly<FFragment_Objective_Params>,
            TReadWrite<FFragment_Objective_Current>,
            FTag_Objective_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_ObjectiveOwner_HandleRequests>;
        using MarkedDirtyBy = FTag_Objective_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Objective_Params& InParams,
            FFragment_Objective_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOBJECTIVE_API FProcessor_Objective_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Objective_HandleRequests,
            FCk_Handle_Objective,
            TReadWrite<FFragment_Objective_Current>,
            TReadOnly<FFragment_Objective_Params>,
            TReadOnly<FFragment_Objective_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Objective_Setup>;
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
            const FCk_Request_Objective_Start& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Complete& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Fail& InRequest) -> bool;

    private:
        static auto
        DoSetStatus(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            ECk_ObjectiveStatus NewStatus) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKOBJECTIVE_API FProcessor_Objective_EndPlay : public ck_exp::TProcessor<
            FProcessor_Objective_EndPlay,
            FCk_Handle_Objective,
            TReadOnly<FFragment_Objective_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Objective_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed objective's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKOBJECTIVE_API FProcessor_Objective_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Objective_CancelPendingRequests,
        FCk_Handle_Objective,
        ck::TReadOnly<FFragment_Objective_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Objective_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------