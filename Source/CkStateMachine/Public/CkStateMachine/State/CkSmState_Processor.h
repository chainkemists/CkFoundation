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
    class FProcessor_Sm_HandleRequests;
    class FProcessor_Sm_CommitPendingTransition;
    class FProcessor_SmCondition_Exit;

    // --------------------------------------------------------------------------------------------------------------------

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
        // Gates the active phase: runs after the exit cascade + commit, so the rest of the active
        // chain (Evaluate / Tick / FireFinishedSignal) follows downstream.
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
        // Must run AFTER Update, the producer of FTag_SmState_NeedsEvaluation, so the tag added this
        // frame is consumed this frame — otherwise polled transitions react with a 1-frame lag and the
        // first frame after a state becomes Active runs no Evaluate at all.
        using RunAfter = TDepList<FProcessor_Sm_HandleRequests, FProcessor_SmState_Update>;
        using MarkedDirtyBy = FTag_SmState_NeedsEvaluation;
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSTATEMACHINE_API FProcessor_SmState_Exit : public ck_exp::TProcessor<
        FProcessor_SmState_Exit,
        FCk_Handle_SmState,
        FTag_SmState_PendingExit,
        TReadOnly<FFragment_EntityScript_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        // First phase of an SM tick (state -> task -> transition -> condition -> commit), all of
        // which run before any active processor.
        using RunAfter      = TDepList<FProcessor_Sm_HandleRequests>;
        using MarkedDirtyBy = FTag_SmState_PendingExit;
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

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
