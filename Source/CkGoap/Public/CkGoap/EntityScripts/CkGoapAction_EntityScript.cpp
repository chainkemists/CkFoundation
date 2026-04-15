#include "CkGoapAction_EntityScript.h"

// ====================================================================================================================

auto
	UCk_GoapAction_EntityScript::
	DefineAction()
	-> void
{
	// Clear previous state (CDO may be called multiple times if actions are re-registered)
	_Preconditions = ck::goap::FWorldState{};
	_Effects = ck::goap::FWorldState{};
	_Cost = 1.0f;

	DoDefineAction();
}

// ====================================================================================================================

void
	UCk_GoapAction_EntityScript::
	AddPrecondition(
		FGameplayTag InKey,
		bool InValue)
{
	_Preconditions.Set(InKey, InValue);
}

// ====================================================================================================================

void
	UCk_GoapAction_EntityScript::
	AddEffect(
		FGameplayTag InKey,
		bool InValue)
{
	_Effects.Set(InKey, InValue);
}

// ====================================================================================================================

void
	UCk_GoapAction_EntityScript::
	SetCost(
		float InCost)
{
	_Cost = InCost;
}

// ====================================================================================================================
