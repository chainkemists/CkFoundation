#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkHFSM/Condition/CkCondition_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKHFSM_API FProcessor_Condition_Setup
        : public ck_exp::TProcessor<FProcessor_Condition_Setup, FCk_Handle_Condition,
            FFragment_Condition_Params, FFragment_Condition_Current, FTag_Condition_Setup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Condition_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Condition_Enter
        : public ck_exp::TProcessor<FProcessor_Condition_Enter, FCk_Handle_Condition,
            FFragment_Condition_Current, FTag_Condition_Enter, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Condition_Enter;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Condition_Exit
        : public ck_exp::TProcessor<FProcessor_Condition_Exit, FCk_Handle_Condition,
            FFragment_Condition_Current, FTag_Condition_Exit, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Condition_Exit;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Condition_Evaluate
        : public ck_exp::TProcessor<FProcessor_Condition_Evaluate, FCk_Handle_Condition,
            FFragment_Condition_Current, FTag_StateMachine_Evaluate_TransitionOrCondition,
            TExclude<FTag_Condition_Enter>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Evaluate_TransitionOrCondition;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Condition_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Condition_HandleRequests, FCk_Handle_Condition,
            FFragment_Condition_Params, FFragment_Condition_Current, FFragment_Condition_Requests, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Condition_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent,
            const FFragment_Condition_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Condition_Params& InParams,
            FFragment_Condition_Current& InCurrent,
            const FCk_Request_Condition_Command& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Condition_Current& InCurrent,
            const FCk_Request_Condition_MarkResult& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------