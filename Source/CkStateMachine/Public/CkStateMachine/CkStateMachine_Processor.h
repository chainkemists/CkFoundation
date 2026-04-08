#pragma once

#include "CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // SETUP — One-time initialization, auto-start if configured
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_Setup : public ck_exp::TProcessor<
        FProcessor_Sm_Setup,
        FCk_Handle_StateMachine,
        FFragment_Sm_Params,
        FFragment_Sm_Current,
        FTag_Sm_RequiresSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // HANDLE REQUESTS — Process Start/Stop/Pause/Resume/Transition
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Sm_HandleRequests,
        FCk_Handle_StateMachine,
        FFragment_Sm_Params,
        FFragment_Sm_Current,
        FFragment_Sm_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Sm_Setup>;
        using MarkedDirtyBy = FFragment_Sm_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest) -> void;

    private:
        static auto
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> void;

        static auto
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // CONDITION RESET — Reset non-latching conditions at the start of each frame
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmCondition_ResetEveryFrame : public ck_exp::TProcessor<
        FProcessor_SmCondition_ResetEveryFrame,
        FCk_Handle_SmCondition,
        FFragment_SmCondition_Current,
        FTag_EntityScript_HasBegunPlay,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
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
    // CONDITION POLLED — Evaluate polled conditions and write _IsSatisfied
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmCondition_Polled : public ck_exp::TProcessor<
        FProcessor_SmCondition_Polled,
        FCk_Handle_SmCondition,
        FFragment_SmCondition_Current,
        FTag_SmCondition_Polled,
        FTag_EntityScript_HasBegunPlay,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
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

    // ================================================================================================================
    // EVAL TRANSITIONS — Walk SM→state→transitions (by _Order), first winner queues request
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_EvalTransitions : public ck_exp::TProcessor<
        FProcessor_Sm_EvalTransitions,
        FCk_Handle_StateMachine,
        FFragment_Sm_Current,
        FTag_Sm_Running,
        TExclude<FTag_Sm_Paused>,
        TExclude<FTag_Sm_TransitionQueued>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_SmCondition_Polled>;

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
        DoAreAllConditionsSatisfied(
            FCk_Handle InTransitionHandle) -> bool;
    };

    // ================================================================================================================
    // TASK TICK — Tick all tick-mode tasks
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmTask_Tick : public ck_exp::TProcessor<
        FProcessor_SmTask_Tick,
        FCk_Handle_SmTask,
        FFragment_SmTask_Current,
        FTag_SmTask_Tick,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Sm_EvalTransitions>;

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
    // ENDPLAY — Cleanup on SM entity destruction
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_EndPlay : public ck_exp::TProcessor<
        FProcessor_Sm_EndPlay,
        FCk_Handle_StateMachine,
        FFragment_Sm_Current,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
