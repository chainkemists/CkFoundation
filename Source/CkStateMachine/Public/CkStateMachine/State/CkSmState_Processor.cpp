#include "CkSmState_Processor.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmState_Evaluate);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_SmState_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
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
                    UCk_Utils_SmTransition_UE::Request_StartEvaluating(InTransition);
                    return ECk_Record_ForEachIterationResult::Break;
                }
                case ECk_SmTransitionResult::Fail:
                {
                    return ECk_Record_ForEachIterationResult::Continue;
                }
                case ECk_SmTransitionResult::Pass:
                {
                    const auto TargetStateClass = UCk_Utils_SmTransition_UE::Get_TargetStateClass(InTransition);

                    UCk_Utils_SmState_UE::TryCheckTransitionBreakpoint(StateMachine, TargetStateClass);

                    UCk_Utils_StateMachine_UE::Request_Transition(StateMachine, TargetStateClass);
                    StateMachine.AddOrGet<FTag_Sm_TransitionQueued>();

                    UCk_Utils_SmState_UE::TryRecordLastFiredTransition(StateMachine, InTransition);

                    sm::Verbose(TEXT("SM [{}] transition queued to [{}]"),
                        StateMachine, TargetStateClass->GetName());

                    return ECk_Record_ForEachIterationResult::Break;
                }
            }

            return ECk_Record_ForEachIterationResult::Continue;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
