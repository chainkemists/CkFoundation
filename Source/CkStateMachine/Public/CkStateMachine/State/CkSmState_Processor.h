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
    // Forward declarations for RunAfter dependencies
    class FProcessor_SmTransition_Evaluate;

    // ================================================================================================================
    // STATE UPDATE — Add NeedsEvaluation every frame for Ticking (non-EventDriven) states
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmState_Update : public ck_exp::TProcessor<
        FProcessor_SmState_Update,
        FCk_Handle_SmState,
        FTag_SmState_Active,
        TExclude<FTag_SmState_FullyEventDriven>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // ================================================================================================================
    // STATE EVALUATE — Walk the active state's transitions in priority order and fire the first that passes
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmState_Evaluate : public ck_exp::TProcessor<
        FProcessor_SmState_Evaluate,
        FCk_Handle_SmState,
        FTag_SmState_Active,
        FTag_SmState_NeedsEvaluation,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTransition_Evaluate, FProcessor_SmState_Update>;
        using MarkedDirtyBy = FTag_SmState_NeedsEvaluation;

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
