#include "CkSmState_Processor.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/CkStateMachine_Stats.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmState_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmState_Evaluate);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmState_Exit);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("SmState::Update"), STAT_SmState_Update, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmState::Exit"), STAT_SmState_Exit, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmState::Evaluate (proc)"), STAT_SmState_EvaluateProc, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmState::Evaluate (transition walk)"), STAT_SmState_Evaluate, STATGROUP_CkStateMachine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sm States Evaluated"), STAT_SmStatesEvaluated, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // STATE UPDATE
    // ================================================================================================================

    auto
        FProcessor_SmState_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmState_Update);

        UCk_Utils_SmState_UE::Request_Evaluate(InHandle);
    }

    // ================================================================================================================
    // STATE EXIT
    // ================================================================================================================

    auto
        FProcessor_SmState_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmState_Exit);

        UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_Entry(InHandle,
        [](FCk_Handle_SmTask InTask)
        {
            UCk_Utils_SmTask_UE::Request_Exit(InTask);
        });

        UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_Entry(InHandle,
        [](FCk_Handle_SmTransition InTransition)
        {
            UCk_Utils_SmTransition_UE::Request_Exit(InTransition);
        });

        if (auto* Script = Cast<UCk_SmState_EntityScript>(InScriptFragment.Get_Script().Get());
            ck::IsValid(Script))
        {
            const auto SmHandle = UCk_Utils_SmState_UE::Get_OwningStateMachine(InHandle);
            const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
            Script->ExitState(InHandle, NetContext);
        }

        InHandle.Try_Remove<FTag_SmState_PendingExit>();
    }

    // ================================================================================================================
    // STATE EVALUATE
    // ================================================================================================================

    auto
        FProcessor_SmState_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmState_EvaluateProc);

        InHandle.Try_Remove<FTag_SmState_NeedsEvaluation>();

        // A CkSnapshot restore brings SM state-entities back with their FTag_SmState_Active tag (the generic tag
        // section restores every tag present at capture), but FFragment_Sm_OwningStateMachine (the owning-SM
        // EntityHolder) is NOT snapshotable — so a restored state is orphaned. FProcessor_Sm_RestoreRedrive
        // recreates the live state graph fresh, so this stale orphan must not be evaluated. Strip its Active tag
        // up front so it drops out of the loop — mirrors the redrive's virgin-reset tag strip on the SM entity
        // (CkStateMachine_Processor.cpp:880-887) — and avoids the Get_StoredEntity ensure on the missing holder.
        if (NOT TUtils_Sm_OwningStateMachine::Has(InHandle))
        {
            InHandle.Try_Remove<FTag_SmState_Active>();
            return;
        }

        auto StateMachine = TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);

        if (ck::Is_NOT_Valid(StateMachine))
        { return; }

        if (NOT StateMachine.Has<FTag_Sm_Running>()
            || StateMachine.Has<FTag_Sm_Paused>()
            || StateMachine.Has<FTag_Sm_TransitionQueued>())
        { return; }

        // Transition-authority gate: only the machine that DECIDES this SM's transitions walks them.
        // Non-authority machines (the server of an OwningClientAuth SM, all non-owning clients) follow
        // the replicated/relayed transitions and must NOT self-evaluate — else they fight the relay
        // (e.g. a server self-reverting a relayed sub-SM when a release condition is locally true).
        // Gating here prevents the decision outright: nothing is queued, so nothing is dropped later.
        if (NOT ck::statemachine::Get_IsTransitionAuthority(StateMachine))
        { return; }

        // ---- Walk transitions in record order (insertion order = priority order) ----

        INC_DWORD_STAT(STAT_SmStatesEvaluated);
        SCOPE_CYCLE_COUNTER(STAT_SmState_Evaluate);

        UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(InHandle,
        [&](FCk_Handle_SmTransition InTransition) -> ECk_Record_ForEachIterationResult
        {
            switch (UCk_Utils_SmTransition_UE::Get_EvaluationResult(InTransition))
            {
                case ECk_SmTransitionResult::Undetermined:
                {
                    sm::VeryVerbose(TEXT("State [{}] — transition [{}] Undetermined, starting evaluation"), InHandle, InTransition);
                    UCk_Utils_SmTransition_UE::Request_StartEvaluating(InTransition);
                    return ECk_Record_ForEachIterationResult::Break;
                }
                case ECk_SmTransitionResult::Fail:
                {
                    sm::VeryVerbose(TEXT("State [{}] — transition [{}] Fail, resetting for next evaluation cycle"), InHandle, InTransition);
                    UCk_Utils_SmTransition_UE::Request_ResetTransition(InTransition);
                    return ECk_Record_ForEachIterationResult::Continue;
                }
                case ECk_SmTransitionResult::Pass:
                {
                    const auto TargetStateClass = UCk_Utils_SmTransition_UE::Get_TargetStateClass(InTransition);

                    UCk_Utils_SmState_UE::TryCheckTransitionBreakpoint(StateMachine, TargetStateClass);
                    UCk_Utils_StateMachine_UE::Request_Transition(StateMachine, TargetStateClass);
                    UCk_Utils_SmState_UE::TryRecordLastFiredTransition(StateMachine, InTransition);

                    sm::Verbose(TEXT("SM [{}] transition queued to [{}]"), StateMachine, TargetStateClass->GetName());

                    return ECk_Record_ForEachIterationResult::Break;
                }
            }

            return ECk_Record_ForEachIterationResult::Continue;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
