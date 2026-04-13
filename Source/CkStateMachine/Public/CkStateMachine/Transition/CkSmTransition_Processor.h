#pragma once

#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies declared in CkSmCondition_Processor.h
    class FProcessor_SmCondition_Polled;

    // ================================================================================================================
    // TRANSITION EVALUATE — AND all condition results into the transition's _Result
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTransition_Evaluate : public ck_exp::TProcessor<
        FProcessor_SmTransition_Evaluate,
        FCk_Handle_SmTransition,
        TReadWrite<FFragment_SmTransition_Current>,
        FTag_SmTransition_Evaluating,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmCondition_Polled>;
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

}

// --------------------------------------------------------------------------------------------------------------------
