#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_SmCondition_EventDriven::
    MarkSatisfied()
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

void
    UCk_SmCondition_EventDriven::
    MarkUnsatisfied()
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
