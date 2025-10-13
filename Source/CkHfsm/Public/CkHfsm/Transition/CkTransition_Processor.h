#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkHfsm/Transition/CkTransition_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKHFSM_API FProcessor_Transition_Setup
        : public ck_exp::TProcessor<FProcessor_Transition_Setup, FCk_Handle_Transition,
            FFragment_Transition_Params, FFragment_Transition_Current, FTag_Transition_Setup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transition_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams,
            FFragment_Transition_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Transition_Enter
        : public ck_exp::TProcessor<FProcessor_Transition_Enter, FCk_Handle_Transition,
            FFragment_Transition_Params, FTag_Transition_Enter, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transition_Enter;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Transition_Exit
        : public ck_exp::TProcessor<FProcessor_Transition_Exit, FCk_Handle_Transition,
            FFragment_Transition_Params, FTag_Transition_Exit, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transition_Exit;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transition_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_Transition_Evaluate
        : public ck_exp::TProcessor<FProcessor_Transition_Evaluate, FCk_Handle_Transition,
            FFragment_Transition_Params, FTag_StateMachine_Evaluate_TransitionOrCondition,
            TExclude<FTag_Transition_Enter>, CK_IGNORE_PENDING_KILL>
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
            const FFragment_Transition_Params& InParams) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------