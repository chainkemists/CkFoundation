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
    // Forward declaration for RunAfter dependency declared in CkStateMachine_Processor.h
    class FProcessor_Sm_HandleRequests;

    // ================================================================================================================
    // CONDITION RESET — Reset non-latching conditions at the start of each frame
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmCondition_ResetEveryFrame : public ck_exp::TProcessor<
        FProcessor_SmCondition_ResetEveryFrame,
        FCk_Handle_SmCondition,
        ck::TReadWrite<FFragment_SmCondition_Current>,
        FTag_SmCondition_ResetsEveryFrame,
        FTag_EntityScript_HasBegunPlay,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Sm_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // CONDITION POLLED — Evaluate polled conditions and write _Result (Pass/Fail)
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmCondition_Polled : public ck_exp::TProcessor<
        FProcessor_SmCondition_Polled,
        FCk_Handle_SmCondition,
        ck::TReadWrite<FFragment_SmCondition_Current>,
        FTag_SmCondition_Polled,
        FTag_EntityScript_HasBegunPlay,
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
}

// --------------------------------------------------------------------------------------------------------------------
