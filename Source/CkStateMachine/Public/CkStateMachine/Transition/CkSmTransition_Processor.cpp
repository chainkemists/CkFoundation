#include "CkSmTransition_Processor.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment_Data.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_EvaluateFromConditions);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TRANSITION EVALUATE FROM CONDITIONS
    // Mirrors the reference project's ConditionGroup_Evaluate: iterates conditions one at a time.
    // ================================================================================================================

    auto
        FProcessor_SmTransition_EvaluateFromConditions::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTransition_Current& InCurrent)
        -> void
    {
        auto HasAnyConditions = false;
        auto WaitingForCondition = false;
        auto AnyFail = false;

        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InHandle,
            [&](FCk_Handle_SmCondition InCondition) -> ECk_Record_ForEachIterationResult
            {
                if (NOT InCondition.Has<FFragment_SmCondition_Current>())
                { return ECk_Record_ForEachIterationResult::Continue; }

                HasAnyConditions = true;

                switch (UCk_Utils_SmCondition_UE::Get_EvaluationResult(InCondition))
                {
                case ECk_SmConditionResult::Undetermined:
                {
                    UCk_Utils_SmCondition_UE::Request_StartOrResumeEvaluating(InCondition);
                    WaitingForCondition = true;
                    return ECk_Record_ForEachIterationResult::Break;
                }
                case ECk_SmConditionResult::Pass:
                {
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(InCondition);
                    return ECk_Record_ForEachIterationResult::Continue;
                }
                case ECk_SmConditionResult::Fail:
                {
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(InCondition);
                    AnyFail = true;
                    return ECk_Record_ForEachIterationResult::Break;
                }
                }

                return ECk_Record_ForEachIterationResult::Continue;
            });

        // ----

        if (NOT HasAnyConditions)
        {
            InCurrent._Result = ECk_SmTransitionResult::Pass;
            InHandle.Remove<FTag_SmTransition_Evaluating>();
            return;
        }

        if (WaitingForCondition)
        { return; }

        InCurrent._Result = AnyFail
            ? ECk_SmTransitionResult::Fail
            : ECk_SmTransitionResult::Pass;

        InHandle.Remove<FTag_SmTransition_Evaluating>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
