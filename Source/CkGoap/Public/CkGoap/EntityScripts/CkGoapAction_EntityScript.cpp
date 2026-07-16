#include "CkGoapAction_EntityScript.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_GoapAction_EntityScript::
	Get_ActionTagForClass(
		TSubclassOf<UCk_GoapAction_EntityScript> InClass)
	-> FGameplayTag
{
	CK_ENSURE_IF_NOT(ck::IsValid(InClass),
		TEXT("Invalid action class in Get_ActionTagForClass"))
	{ return {}; }

	return UCk_Utils_Object_UE::Get_TagFromClassName(
		InClass,
		ck::Format_UE(TEXT("Auto-generated action tag for {}"), InClass));
}

auto
	UCk_GoapAction_EntityScript::
	Get_ActionTag() const
	-> FGameplayTag
{
	return Get_ActionTagForClass(GetClass());
}

// --------------------------------------------------------------------------------------------------------------------
