#include "CkSmState_Processor.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkStateMachine/CkStateMachine_Log.h"
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
            Script->ExitState(InHandle);
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
        InHandle.Try_Remove<FTag_SmState_NeedsEvaluation>();

        auto StateMachine = TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);

        if (ck::Is_NOT_Valid(StateMachine))
        { return; }

        if (NOT StateMachine.Has<FTag_Sm_Running>()
            || StateMachine.Has<FTag_Sm_Paused>()
            || StateMachine.Has<FTag_Sm_TransitionQueued>())
        { return; }

        // ---- Walk transitions in record order (insertion order = priority order) ----

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
