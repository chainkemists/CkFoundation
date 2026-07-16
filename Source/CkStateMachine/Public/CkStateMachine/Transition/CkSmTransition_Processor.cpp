#include "CkSmTransition_Processor.h"

#include "CkStateMachine/CkStateMachine_Stats.h"
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

DECLARE_CYCLE_STAT(TEXT("SmTransition::Exit"), STAT_SmTransition_Exit, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmTransition::Evaluate"), STAT_SmTransition_Evaluate, STATGROUP_CkStateMachine);

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
        SCOPE_CYCLE_COUNTER(STAT_SmTransition_Exit);

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
        SCOPE_CYCLE_COUNTER(STAT_SmTransition_Evaluate);

        InHandle.Remove<MarkedDirtyBy>();

        // Authority gating (spec §5/§6): condition evaluation drives transitions. Non-authority
        // machines must not evaluate — they receive transitions via replication/relay. Keep the
        // dirty marker removed (above) so we don't loop; the rep-driven replay path will
        // commit the transition on its own schedule. Uses the shared transition-authority
        // predicate — the prior inline gate only skipped NonOwningClient and non-OwningClientAuth
        // OwningClient, which left the server of a relayed OwningClientAuth sub-SM evaluating.
        const auto SmHandle = UCk_Utils_SmTransition_UE::Get_OwningStateMachine(InHandle);

        if (NOT ck::statemachine::Get_IsTransitionAuthority(SmHandle))
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

        // Single-pass eager AND: arm every Undetermined condition now instead of one per
        // evaluate cycle. All of them get evaluated in the same polled main-pass sweep, whose
        // unconditional parent wake re-runs this evaluate in the pump — the whole decision
        // resolves within one frame instead of one condition per frame. Fail still wins
        // immediately. Polled Evaluate is const (pure predicate), so eager evaluation of a
        // condition that a failing sibling would previously have short-circuited is safe.
        auto AnyUndetermined = false;

        for (auto Condition : Conditions)
        {
            switch (UCk_Utils_SmCondition_UE::Get_EvaluationResult(Condition))
            {
                case ECk_SmConditionResult::Undetermined:
                {
                    sm::VeryVerbose(TEXT("Transition [{}] — condition [{}] Undetermined, activating"), InHandle, Condition);
                    UCk_Utils_SmCondition_UE::Request_StartOrResumeEvaluating(Condition);
                    AnyUndetermined = true;
                    break;
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

        if (AnyUndetermined)
        { return; }

        sm::VeryVerbose(TEXT("Transition [{}] — all conditions Pass"), InHandle);
        UCk_Utils_SmTransition_UE::Request_UpdateTransitionResult(InHandle, ECk_SmTransitionResult::Pass);
        if (ck::IsValid(ParentState))
        {
            UCk_Utils_SmState_UE::Request_Evaluate(ParentState);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
