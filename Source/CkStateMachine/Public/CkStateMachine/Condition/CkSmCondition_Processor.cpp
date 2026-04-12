#include "CkSmCondition_Processor.h"

#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_ResetEveryFrame);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmCondition_Polled);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
        // Gate: only evaluate polled conditions when the parent state is Ticking.
        // Event-driven states never run the polled evaluation loop.
        {
            const auto ParentTransitionHandle = static_cast<FCk_Handle>(
                TUtils_Sm_ParentTransition::Get_StoredEntity(static_cast<FCk_Handle>(InHandle)));
            const auto ParentStateHandle = static_cast<FCk_Handle>(
                TUtils_Sm_ParentState::Get_StoredEntity(ParentTransitionHandle));

            CK_ENSURE_IF_NOT(
                ck::Is_NOT_Valid(ParentStateHandle) || NOT ParentStateHandle.Has<FTag_SmState_EventDriven>(),
                TEXT("Polled condition [{}] is attached to an event-driven state — this is a programmer error. "
                     "Mark the parent state as Ticking or switch this condition to EventDriven mode."), InHandle)
            { return; }

            if (ck::Is_NOT_Valid(ParentStateHandle) || NOT ParentStateHandle.Has<FTag_SmState_Ticking>())
            { return; }
        }

        CK_ENSURE_IF_NOT(InHandle.Has<FFragment_EntityScript_Current>(),
            TEXT("Polled condition entity [{}] is missing FFragment_EntityScript_Current — tag should not have been added without a script"), InHandle)
        { return; }

        const auto& ScriptFragment = InHandle.Get<FFragment_EntityScript_Current>();
        auto* Script = ScriptFragment.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Polled condition entity [{}] has a null script pointer"), InHandle)
        { return; }

        auto* ConditionScript = Cast<UCk_SmCondition_EntityScript>(Script);
        CK_ENSURE_IF_NOT(ck::IsValid(ConditionScript),
            TEXT("Polled condition entity [{}] script is not a UCk_SmCondition_EntityScript — wrong script type added with FTag_SmCondition_Polled"), InHandle)
        { return; }

        InCurrent._Result = ConditionScript->Evaluate()
            ? ECk_SmConditionResult::Pass
            : ECk_SmConditionResult::Fail;
    }
}

// --------------------------------------------------------------------------------------------------------------------
