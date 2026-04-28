#include "CkSmCondition_Processor.h"

#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_Polled.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_ResetEveryFrame);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_Polled);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_Exit);

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
        auto* Script = Cast<UCk_SmCondition_EntityScript>(InScriptFragment.Get_Script().Get());
        if (ck::Is_NOT_Valid(Script))
        { return; }

        Script->ExitCondition(InHandle);
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

        InCurrent._Result = ConditionScript->Evaluate(InHandle, InDeltaT)
            ? ECk_SmConditionResult::Pass
            : ECk_SmConditionResult::Fail;

        // Wake the parent transition so it re-checks condition results in the pump

        if (auto ParentTransition = ck::TUtils_Sm_ParentTransition::Get_StoredEntity(InHandle);
            ck::IsValid(ParentTransition) && ParentTransition.Has<ck::FTag_SmTransition_Evaluating>())
        {
            ParentTransition.AddOrGet<ck::FTag_SmTransition_Evaluating>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
