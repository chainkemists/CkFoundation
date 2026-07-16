#pragma once

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Forward declarations for RunAfter dependencies
    class FProcessor_Sm_HandleRequests;
    class FProcessor_Sm_CommitPendingTransition;
    class FProcessor_SmCondition_Exit;

    // --------------------------------------------------------------------------------------------------------------------
    // STATE UPDATE — Add NeedsEvaluation every frame for Ticking (non-EventDriven) states

    class CKSTATEMACHINE_API FProcessor_SmState_Update : public ck_exp::TProcessor<
        FProcessor_SmState_Update,
        FCk_Handle_SmState,
        FTag_SmState_Active,
        TExclude<FTag_SmState_FullyEventDriven>,
        TExclude<FTag_SmState_PendingExit>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        // Update gates the active phase: it RunAfter the exit cascade + commit, so the rest
        // of the active chain (Evaluate / Tick / FireFinishedSignal) follows downstream.
        using RunAfter = TDepList<FProcessor_Sm_CommitPendingTransition>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // STATE EVALUATE — Walk the active state's transitions in priority order and fire the first that passes

    class CKSTATEMACHINE_API FProcessor_SmState_Evaluate : public ck_exp::TProcessor<
        FProcessor_SmState_Evaluate,
        FCk_Handle_SmState,
        FTag_SmState_Active,
        FTag_SmState_NeedsEvaluation,
        TExclude<FTag_SmState_PendingExit>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        // FProcessor_SmState_Update is the producer of FTag_SmState_NeedsEvaluation (it calls
        // Request_Evaluate every frame for ticking / non-event-driven states). Evaluate must run AFTER
        // Update so the tag added this frame is consumed this frame instead of next frame — otherwise
        // polled-condition transitions react to state with a 1-frame lag and the first frame after a
        // state becomes Active runs no Evaluate at all.
        using RunAfter = TDepList<FProcessor_Sm_HandleRequests, FProcessor_SmState_Update>;
        using MarkedDirtyBy = FTag_SmState_NeedsEvaluation;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // STATE EXIT — Cascade PendingExit to tasks + transitions, then call ExitState

    class CKSTATEMACHINE_API FProcessor_SmState_Exit : public ck_exp::TProcessor<
        FProcessor_SmState_Exit,
        FCk_Handle_SmState,
        FTag_SmState_PendingExit,
        TReadOnly<FFragment_EntityScript_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        // Exit cascade is the first phase of an SM tick (state -> task -> transition ->
        // condition -> commit), all running before any active processor.
        using RunAfter      = TDepList<FProcessor_Sm_HandleRequests>;
        using MarkedDirtyBy = FTag_SmState_PendingExit;

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
