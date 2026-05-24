#include "CkSmTransition_Processor.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkStateMachine/CkStateMachine_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_Evaluate);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmTransition_Exit);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_SmTransition_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle)
        -> void
    {
        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_Entry(InHandle,
        [](FCk_Handle_SmCondition InCondition)
        {
            UCk_Utils_SmCondition_UE::Request_Exit(InCondition);
        });

        InHandle.Try_Remove<FTag_SmTransition_PendingExit>();
    }

    auto
        FProcessor_SmTransition_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTransition_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        // Authority gating (spec §5/§6): condition evaluation drives transitions. Non-authority
        // machines must not evaluate — they receive transitions via replication. Keep the dirty
        // marker removed (above) so we don't loop; the rep-driven replay path (Phase 7) will
        // commit the transition on its own schedule.
        const auto SmHandle = UCk_Utils_SmTransition_UE::Get_OwningStateMachine(InHandle);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);

        if (NetContext == ECk_Sm_NetContext::NonOwningClient)
        { return; }

        if (NetContext == ECk_Sm_NetContext::OwningClient
            && UCk_Utils_StateMachine_UE::Get_AuthorityModel(SmHandle)
                != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        { return; }

        const auto Conditions = UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::Get_ValidEntries(InHandle);
        auto ParentState = TUtils_Sm_ParentState::Get_StoredEntity(InHandle);

        if (Conditions.IsEmpty())
        {
            sm::VeryVerbose(TEXT("Transition [{}] — no conditions, vacuous Pass"), InHandle);
            UCk_Utils_SmTransition_UE::Request_UpdateTransitionResult(InHandle, ECk_SmTransitionResult::Pass);
            if (ck::IsValid(ParentState))
            {
                UCk_Utils_SmState_UE::Request_Evaluate(ParentState);
            }

            return;
        }

        for (auto Condition : Conditions)
        {
            switch (UCk_Utils_SmCondition_UE::Get_EvaluationResult(Condition))
            {
                case ECk_SmConditionResult::Undetermined:
                {
                    sm::VeryVerbose(TEXT("Transition [{}] — condition [{}] Undetermined, activating"), InHandle, Condition);
                    UCk_Utils_SmCondition_UE::Request_StartOrResumeEvaluating(Condition);
                    return;
                }
                case ECk_SmConditionResult::Pass:
                {
                    sm::VeryVerbose(TEXT("Transition [{}] — condition [{}] Pass, continuing"), InHandle, Condition);
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(Condition);
                    break;
                }
                case ECk_SmConditionResult::Fail:
                {
                    sm::VeryVerbose(TEXT("Transition [{}] — condition [{}] Fail, transition Fail"), InHandle, Condition);
                    UCk_Utils_SmCondition_UE::Request_PauseEvaluation(Condition);

                    UCk_Utils_SmTransition_UE::Request_UpdateTransitionResult(InHandle, ECk_SmTransitionResult::Fail);

                    if (ck::IsValid(ParentState))
                    {
                        UCk_Utils_SmState_UE::Request_Evaluate(ParentState);
                    }

                    return;
                }
            }
        }

        sm::VeryVerbose(TEXT("Transition [{}] — all conditions Pass"), InHandle);
        UCk_Utils_SmTransition_UE::Request_UpdateTransitionResult(InHandle, ECk_SmTransitionResult::Pass);
        if (ck::IsValid(ParentState))
        {
            UCk_Utils_SmState_UE::Request_Evaluate(ParentState);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
