#include "CkSmTransition_Processor.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_Evaluate);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_SmTransition_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTransition_Current& InCurrent)
        -> void
    {
        const auto Conditions = UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Get_ValidEntries(InHandle);

        if (Conditions.IsEmpty())
        {
            UCk_Utils_SmTransition_UE::MarkTransitionAs_EvaluationPassed(InHandle);
            return;
        }

        for (auto Condition : Conditions)
        {
            switch (UCk_Utils_SmCondition_UE::Get_EvaluationResult(Condition))
            {
                case ECk_SmConditionResult::Undetermined:
                {
                    UCk_Utils_SmCondition_UE::Request_StartOrResumeEvaluating(Condition);
                    return;
                }
                case ECk_SmConditionResult::Pass:
                {
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(Condition);
                    break;
                }
                case ECk_SmConditionResult::Fail:
                {
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(Condition);
                    UCk_Utils_SmTransition_UE::MarkTransitionAs_EvaluationFailed(InHandle);
                    return;
                }
            }
        }

        UCk_Utils_SmTransition_UE::MarkTransitionAs_EvaluationPassed(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
