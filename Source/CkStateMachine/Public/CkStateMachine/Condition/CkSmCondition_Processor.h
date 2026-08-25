#pragma once

#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies
    class FProcessor_SmTransition_Evaluate;
    class FProcessor_SmTransition_Exit;

    // --------------------------------------------------------------------------------------------------------------------
    // CONDITION RESET — Reset non-latching conditions at the start of each frame

    class CKSTATEMACHINE_API FProcessor_SmCondition_ResetEveryFrame : public ck_exp::TProcessor<
        FProcessor_SmCondition_ResetEveryFrame,
        FCk_Handle_SmCondition,
        TReadWrite<FFragment_SmCondition_Current>,
        FTag_SmCondition_Polled,
        FTag_SmCondition_Evaluating,
        TExclude<FTag_SmCondition_PendingExit>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTransition_Evaluate>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // CONDITION POLLED — Evaluate polled conditions and write _Result (Pass/Fail)

    class CKSTATEMACHINE_API FProcessor_SmCondition_Polled : public ck_exp::TProcessor<
        FProcessor_SmCondition_Polled,
        FCk_Handle_SmCondition,
        TReadWrite<FFragment_SmCondition_Current>,
        FTag_SmCondition_Polled,
        FTag_SmCondition_Evaluating,
        TExclude<FTag_SmCondition_PendingExit>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmCondition_ResetEveryFrame>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // CONDITION EXIT — Call ExitCondition on each condition tagged with FTag_SmCondition_PendingExit

    class CKSTATEMACHINE_API FProcessor_SmCondition_Exit : public ck_exp::TProcessor<
        FProcessor_SmCondition_Exit,
        FCk_Handle_SmCondition,
        FTag_SmCondition_PendingExit,
        TReadOnly<FFragment_EntityScript_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        using RunAfter      = TDepList<FProcessor_SmTransition_Exit>;
        using MarkedDirtyBy = FTag_SmCondition_PendingExit;
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptFragment) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
