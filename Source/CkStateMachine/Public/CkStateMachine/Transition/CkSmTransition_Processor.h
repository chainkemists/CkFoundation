#pragma once

#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies
    class FProcessor_SmState_Evaluate;
    class FProcessor_SmTask_Exit;

    // ================================================================================================================
    // TRANSITION EVALUATE — AND all condition results into the transition's _Result
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTransition_Evaluate : public ck_exp::TProcessor<
        FProcessor_SmTransition_Evaluate,
        FCk_Handle_SmTransition,
        TReadWrite<FFragment_SmTransition_Current>,
        FTag_SmTransition_Evaluating,
        TExclude<FTag_SmTransition_PendingExit>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmState_Evaluate>;
        using MarkedDirtyBy = FTag_SmTransition_Evaluating;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTransition_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // TRANSITION EXIT — Cascade PendingExit to conditions (transitions have no entity script)
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTransition_Exit : public ck_exp::TProcessor<
        FProcessor_SmTransition_Exit,
        FCk_Handle_SmTransition,
        FTag_SmTransition_PendingExit,
        CK_IF_END_PLAY>
    {
    public:
        using Group    = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_SmTask_Exit>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------
