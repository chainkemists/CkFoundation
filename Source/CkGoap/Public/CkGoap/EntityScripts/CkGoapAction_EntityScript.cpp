#include "CkGoapAction_EntityScript.h"

// ====================================================================================================================

auto
	UCk_GoapAction_EntityScript::
	DefineAction()
	-> void
{
	_Preconditions.Reset();
	_Effects.Reset();
	_Cost = 1.0f;

	DoDefineAction();
}

// ====================================================================================================================

void
	UCk_GoapAction_EntityScript::
	AddPrecondition(FGameplayTag InKey, bool InValue)
{
	_Preconditions.Add({InKey, InValue});
}

void
	UCk_GoapAction_EntityScript::
	AddEffect(FGameplayTag InKey, bool InValue)
{
	_Effects.Add({InKey, InValue});
}

void
	UCk_GoapAction_EntityScript::
	SetCost(float InCost)
{
	_Cost = InCost;
}

// ====================================================================================================================
