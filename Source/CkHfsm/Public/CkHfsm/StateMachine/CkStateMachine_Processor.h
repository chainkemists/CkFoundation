#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkHfsm/StateMachine/CkStateMachine_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKHFSM_API FProcessor_StateMachine_Setup
        : public ck_exp::TProcessor<FProcessor_StateMachine_Setup, FCk_Handle_StateMachine,
            FFragment_StateMachine_Params, FFragment_StateMachine_Current,
            FTag_StateMachine_Setup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateMachine_Params& InParams,
            FFragment_StateMachine_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_StateMachine_Enter
        : public ck_exp::TProcessor<FProcessor_StateMachine_Enter, FCk_Handle_StateMachine,
            FFragment_StateMachine_Params, FFragment_StateMachine_Current,
            FTag_StateMachine_Enter, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Enter;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_StateMachine_Params& InParams,
            FFragment_StateMachine_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_StateMachine_Exit
        : public ck_exp::TProcessor<FProcessor_StateMachine_Exit, FCk_Handle_StateMachine,
            FFragment_StateMachine_Current, FTag_StateMachine_Exit, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Exit;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_StateMachine_Transition
        : public ck_exp::TProcessor<FProcessor_StateMachine_Transition, FCk_Handle_StateMachine,
            FFragment_StateMachine_Current, FTag_StateMachine_Transition, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Transition;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKHFSM_API FProcessor_StateMachine_Evaluate
        : public ck_exp::TProcessor<FProcessor_StateMachine_Evaluate, FCk_Handle_StateMachine,
            FFragment_StateMachine_Current, FTag_StateMachine_Evaluate_StateMachine,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_StateMachine_Evaluate_StateMachine;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_StateMachine_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------