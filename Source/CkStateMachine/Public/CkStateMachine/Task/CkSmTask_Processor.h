#pragma once

#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declaration for RunAfter dependency declared in CkSmState_Processor.h
    class FProcessor_SmState_Evaluate;

    // ================================================================================================================
    // TASK TICK — Tick all tick-mode tasks
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_Tick : public ck_exp::TProcessor<
        FProcessor_SmTask_Tick,
        FCk_Handle_SmTask,
        ck::TReadWrite<FFragment_SmTask_Current>,
        FTag_SmTask_Tick,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_SmState_Evaluate>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTask_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // TASK FIRE FINISHED SIGNAL — Broadcast OnSmTaskFinished for tasks that transitioned to a terminal result
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_FireFinishedSignal : public ck_exp::TProcessor<
        FProcessor_SmTask_FireFinishedSignal,
        FCk_Handle_SmTask,
        ck::TReadOnly<FFragment_SmTask_Current>,
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
}

// --------------------------------------------------------------------------------------------------------------------
