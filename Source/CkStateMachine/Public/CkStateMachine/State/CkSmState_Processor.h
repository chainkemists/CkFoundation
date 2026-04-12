#pragma once

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependency declared in CkSmTransition_Processor.h
    class FProcessor_SmTransition_EvaluateFromConditions;

    // ================================================================================================================
    // STATE EVALUATE — Walk the active state's transitions in priority order and fire the first that passes
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmState_Evaluate : public ck_exp::TProcessor<
        FProcessor_SmState_Evaluate,
        FCk_Handle_SmState,
        FTag_SmState_Active,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTransition_EvaluateFromConditions>;
        using MarkedDirtyBy = FTag_SmTransition_Evaluating;

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
