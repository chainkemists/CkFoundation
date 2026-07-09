#include "CkSmCondition_Processor.h"

#include "CkStateMachine/CkStateMachine_Stats.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_Polled.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_ResetEveryFrame);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_Polled);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_Exit);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("SmCondition::Exit"), STAT_SmCondition_Exit, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmCondition::ResetEveryFrame"), STAT_SmCondition_ResetEveryFrame, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmCondition::Polled (proc)"), STAT_SmCondition_PolledProc, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmCondition::Evaluate (user)"), STAT_SmCondition_Evaluate, STATGROUP_CkStateMachine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sm Conditions Evaluated"), STAT_SmConditionsEvaluated, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // CONDITION EXIT
    // ================================================================================================================

    auto
        FProcessor_SmCondition_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmCondition_Exit);

        auto* Script = Cast<UCk_SmCondition_EntityScript>(InScriptFragment.Get_Script().Get());
        if (ck::Is_NOT_Valid(Script))
        { return; }

        const auto SmHandle = UCk_Utils_SmCondition_UE::Get_OwningStateMachine(InHandle);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
        Script->ExitCondition(InHandle, NetContext);
        InHandle.Try_Remove<FTag_SmCondition_PendingExit>();
    }

    // ================================================================================================================
    // CONDITION RESET
    // ================================================================================================================

    auto
        FProcessor_SmCondition_ResetEveryFrame::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmCondition_ResetEveryFrame);

        InCurrent._Result = ECk_SmConditionResult::Undetermined;
    }

    // ================================================================================================================
    // CONDITION POLLED
    // ================================================================================================================

    auto
        FProcessor_SmCondition_Polled::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmCondition_Current& InCurrent)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmCondition_PolledProc);

        CK_ENSURE_IF_NOT(InHandle.Has<FFragment_EntityScript_Current>(),
            TEXT("Polled condition entity [{}] is missing FFragment_EntityScript_Current — tag should not have been added without a script"), InHandle)
        { return; }

        const auto& ScriptFragment = InHandle.Get<FFragment_EntityScript_Current>();
        auto* Script = ScriptFragment.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Polled condition entity [{}] has a null script pointer"), InHandle)
        { return; }

        auto* ConditionScript = Cast<UCk_SmCondition_Polled>(Script);
        CK_ENSURE_IF_NOT(ck::IsValid(ConditionScript),
            TEXT("Polled condition entity [{}] script is not a UCk_SmCondition_Polled — wrong script type added with FTag_SmCondition_Polled"), InHandle)
        { return; }

        // Authority gating: polled conditions evaluate user-defined predicates that often have
        // observable side effects (timers, sensors). Only the SM's transition authority evaluates.
        // Uses the shared predicate so the Server-of-OwningClientAuth case is covered too (a relayed
        // sub-SM is OwningClientAuth on the server, which must follow the relay, not poll) — the prior
        // inline gate only skipped NonOwningClient and non-OwningClientAuth OwningClient.
        const auto SmHandle = UCk_Utils_SmCondition_UE::Get_OwningStateMachine(InHandle);

        if (NOT ck::statemachine::Get_IsTransitionAuthority(SmHandle))
        { return; }

        INC_DWORD_STAT(STAT_SmConditionsEvaluated);

        {
            SCOPE_CYCLE_COUNTER(STAT_SmCondition_Evaluate);

            InCurrent._Result = ConditionScript->Evaluate(InHandle, InDeltaT)
                ? ECk_SmConditionResult::Pass
                : ECk_SmConditionResult::Fail;
        }

        // Wake the parent transition so it re-checks condition results in the pump. Unconditional,
        // mirroring the event-driven path (Request_UpdateConditionResult): AddOrGet bumps the
        // dirty-marker version even when the tag already exists. Gating this on the transition
        // still holding FTag_SmTransition_Evaluating made the wake dead code — the transition
        // evaluator removes that tag at the top of its own run earlier in the same frame, so a
        // freshly-computed result sat unconsumed until the next frame's evaluate cycle.
        if (auto ParentTransition = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InHandle);
            ck::IsValid(ParentTransition) && NOT ParentTransition.Has<ck::FTag_SmTransition_PendingExit>())
        {
            ParentTransition.AddOrGet<ck::FTag_SmTransition_Evaluating>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
