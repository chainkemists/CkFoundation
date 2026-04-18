#include "CkGoapGoal_EntityScript.h"

// ====================================================================================================================

auto
	UCk_GoapGoal_EntityScript::
	DefineGoal()
	-> void
{
	_Conditions.Reset();
	_Priority = 0;

	DoDefineGoal();
}

// ====================================================================================================================

void
	UCk_GoapGoal_EntityScript::
	AddCondition(FGameplayTag InKey, bool InValue)
{
	_Conditions.Add({InKey, InValue});
}

void
	UCk_GoapGoal_EntityScript::
	SetPriority(int32 InPriority)
{
	_Priority = InPriority;
}

// ====================================================================================================================
