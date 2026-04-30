#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EventDriven::
    EnterCondition(
        FCk_Handle_SmCondition InHandle)
    -> void
{
    // Resting state for event-driven conditions is Fail (not yet satisfied),
    // not Undetermined. This unblocks FProcessor_SmState_Evaluate from getting
    // stuck on the first Undetermined transition: with Fail-resting conditions,
    // a first-declared event-driven transition resolves to Fail in the same
    // pump cycle, lets the state evaluator move past it on its next walk, and
    // a later sibling transition that has already resolved Pass (or resolves
    // Pass mid-cycle) wins the race — even if it was declared after the
    // slower one.
    //
    // Use Request_SetInitialResult (direct fragment write, no parent wake-up)
    // rather than Request_UpdateConditionResult: at construction the parent
    // transition will be activated lazily by the state evaluator's normal
    // cascade when it walks the active state, so eagerly bumping the parent's
    // Evaluating tag for every condition's resting write across the freshly-
    // built graph just produces redundant pump work. Polled conditions don't
    // pay this cost (they don't write a resting value), and we want event-
    // driven to match. Subsequent value-changed paths (MarkSatisfied /
    // MarkUnsatisfied) keep using Request_UpdateConditionResult, which DOES
    // wake the parent — those calls are the real "value just changed, re-
    // evaluate now" cases where the wake-up is correct.
    //
    // Set BEFORE Super::EnterCondition fires DoEnterCondition so that any
    // synchronous MarkSatisfied calls from a subclass take effect (the call
    // order is: this Fail write → DoEnterCondition runs → optional sync
    // MarkSatisfied → final result is Pass).
    //
    // Note on _NegateResult: we set Fail literally (not via MarkUnsatisfied),
    // so the resting value is not inverted by negation. After the event fires,
    // MarkSatisfied with negate maps to Fail, so the transition still doesn't
    // fire. Negation of an event-driven condition has no meaningful gameplay
    // shape; we keep it observably consistent with prior behavior.
    UCk_Utils_SmCondition_UE::Request_SetInitialResult(
        InHandle, ECk_SmConditionResult::Fail);

    Super::EnterCondition(InHandle);
}

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
