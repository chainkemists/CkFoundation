#pragma once

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies
    class FProcessor_SmCondition_Polled;
    class FProcessor_SmState_Exit;

    // ================================================================================================================
    // TASK TICK — Tick all tick-mode tasks
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_Tick : public ck_exp::TProcessor<
        FProcessor_SmTask_Tick,
        FCk_Handle_SmTask,
        TReadWrite<FFragment_SmTask_Current>,
        TReadOnly<FFragment_EntityScript_Current>,
        FTag_SmTask_Tick,
        FTag_SmTask_Active,
        TExclude<FTag_SmTask_PendingExit>,
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
            FFragment_SmTask_Current& InCurrent,
            const FFragment_EntityScript_Current& InScriptFragment) -> void;
    };

    // ================================================================================================================
    // TASK FIRE FINISHED SIGNAL — Broadcast OnSmTaskFinished for tasks that transitioned to a terminal result
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_FireFinishedSignal : public ck_exp::TProcessor<
        FProcessor_SmTask_FireFinishedSignal,
        FCk_Handle_SmTask,
        TReadOnly<FFragment_SmTask_Current>,
        FTag_SmTask_ResultDirty,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmTask_Tick>;
        using MarkedDirtyBy = FTag_SmTask_ResultDirty;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmTask_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // TASK EXIT — Call ExitTask on each task tagged with FTag_SmTask_PendingExit
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_Exit : public ck_exp::TProcessor<
        FProcessor_SmTask_Exit,
        FCk_Handle_SmTask,
        FTag_SmTask_PendingExit,
        TReadOnly<FFragment_EntityScript_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group    = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_SmState_Exit>;

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
