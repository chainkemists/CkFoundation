#pragma once

#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies declared in CkSmCondition_Processor.h
    class FProcessor_SmCondition_Polled;

    // ================================================================================================================
    // TRANSITION EVALUATE FROM CONDITIONS — AND all condition results into the transition's _Result
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTransition_EvaluateFromConditions : public ck_exp::TProcessor<
        FProcessor_SmTransition_EvaluateFromConditions,
        FCk_Handle_SmTransition,
        ck::TReadWrite<FFragment_SmTransition_Current>,
        FTag_SmTransition_Evaluating,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmCondition_Polled>;

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
    // TRY FIRE — Walk SM→state→transitions (by _Order), first winner queues request
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTransition_TryFire : public ck_exp::TProcessor<
        FProcessor_SmTransition_TryFire,
        FCk_Handle_StateMachine,
        ck::TReadOnly<FFragment_Sm_Current>,
        FTag_Sm_Running,
        TExclude<FTag_Sm_Paused>,
        TExclude<FTag_Sm_TransitionQueued>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTransition_EvaluateFromConditions>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Current& InCurrent) -> void;

    private:
        static auto
        DoMarkTransitionAs_StartEvaluating(
            FCk_Handle InTransitionHandle) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
