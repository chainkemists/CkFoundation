#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EventDriven::
    MarkSatisfied()
    -> void
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return; }

    auto TypedHandle = UCk_Utils_SmCondition_UE::CastChecked(ConditionHandle);

    // Respect negate: a "satisfied" semantic result maps to Fail when the condition is negated.
    const auto SatisfiedResult = _NegateResult ? ECk_SmConditionResult::Fail : ECk_SmConditionResult::Pass;
    UCk_Utils_SmCondition_UE::Request_UpdateConditionResult(TypedHandle, SatisfiedResult);
}

auto
    UCk_SmCondition_EventDriven::
    MarkUnsatisfied()
    -> void
{
    auto ConditionHandle = DoGet_ScriptEntity();

    if (ck::Is_NOT_Valid(ConditionHandle))
    { return; }

    auto TypedHandle = UCk_Utils_SmCondition_UE::CastChecked(ConditionHandle);

    // Respect negate: an "unsatisfied" semantic result maps to Pass when the condition is negated.
    const auto UnsatisfiedResult = _NegateResult ? ECk_SmConditionResult::Pass : ECk_SmConditionResult::Fail;
    UCk_Utils_SmCondition_UE::Request_UpdateConditionResult(TypedHandle, UnsatisfiedResult);
}

// --------------------------------------------------------------------------------------------------------------------
