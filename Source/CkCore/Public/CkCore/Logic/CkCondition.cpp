#include "CkCondition.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Condition::OnEnter() -> void
{
	DoOnEnter();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Condition::Evaluate(FCk_Time InDeltaT) -> ECk_ConditionResult
{
	ECk_ConditionResult Result = DoEvaluate(InDeltaT);

	if (_Negate)
	{
		Result = (Result == ECk_ConditionResult::Pass)
			? ECk_ConditionResult::Fail
			: ECk_ConditionResult::Pass;
	}

	return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Condition::OnExit() -> void
{
	DoOnExit();
}

// --------------------------------------------------------------------------------------------------------------------
