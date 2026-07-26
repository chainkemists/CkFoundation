#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Net/CkStateMachine_NetContext.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_EventDriven::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    // Resting state is Fail (literally, not via MarkUnsatisfied, so negation does not invert it) and
    // is written BEFORE Super so a subclass's synchronous MarkSatisfied still wins. Setter choice is
    // deliberate — rationale in CkStateMachine/CLAUDE.md § Event-driven condition resting state.
    UCk_Utils_SmCondition_UE::Request_SetInitialResult(
        InHandle, ECk_SmConditionResult::Fail);

    Super::EnterCondition(InHandle, InNetContext);
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

    const auto UnsatisfiedResult = _NegateResult ? ECk_SmConditionResult::Pass : ECk_SmConditionResult::Fail;
    UCk_Utils_SmCondition_UE::Request_UpdateConditionResult(TypedHandle, UnsatisfiedResult);
}

// --------------------------------------------------------------------------------------------------------------------
