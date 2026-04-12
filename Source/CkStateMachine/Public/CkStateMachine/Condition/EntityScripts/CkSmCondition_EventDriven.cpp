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
    if (_NegateResult)
    {
        UCk_Utils_SmCondition_UE::MarkConditionAs_Unsatisfied(TypedHandle);
    }
    else
    {
        UCk_Utils_SmCondition_UE::MarkConditionAs_Satisfied(TypedHandle);
    }
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
    if (_NegateResult)
    {
        UCk_Utils_SmCondition_UE::MarkConditionAs_Satisfied(TypedHandle);
    }
    else
    {
        UCk_Utils_SmCondition_UE::MarkConditionAs_Unsatisfied(TypedHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
