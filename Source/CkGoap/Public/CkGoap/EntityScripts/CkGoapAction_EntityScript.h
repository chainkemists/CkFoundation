#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkGoapAction_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Goap_Setup; class FProcessor_Goap_Action_Setup; }

// --------------------------------------------------------------------------------------------------------------------
//
// GOAP Action definition class.
//
// Subclass and override DefineAction (C++) or DoDefineAction (BP / AS). The
// builder API records preconditions and effects on the CDO. The Setup
// processor reads those out once per entity, resolves tag→FCk_GoapKey via
// the per-entity key registry, and feeds the planner.
//
// Classical (boolean) GOAP: preconditions and effects are (tag, bool) pairs.
// Gameplay code that wants to reason about numerics / enums must project
// those down to booleans before writing to WorldState (e.g. set a
// `HasEnoughFood` tag based on `ActualFood >= Threshold`).
//
// Example:
//     AddPrecondition(Tag_HasKey,      true);
//     AddEffect      (Tag_DoorUnlocked, true);
//     SetCost(1.0f);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGOAP_API UCk_GoapAction_EntityScript : public UObject
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_GoapAction_EntityScript);

public:
	virtual auto
	DefineAction() -> void;

protected:
	UFUNCTION(BlueprintImplementableEvent,
		Category = "Ck|GOAP|Action",
		DisplayName = "Define Action")
	void
	DoDefineAction();

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

protected:
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Add Precondition")
	void
	AddPrecondition(FGameplayTag InKey, bool InValue);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Add Effect")
	void
	AddEffect(FGameplayTag InKey, bool InValue);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Set Cost")
	void
	SetCost(float InCost);

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

public:
	// The action's identity tag, derived from its class name via
	// UCk_Utils_Object_UE::Get_TagFromClassName. Mirrors SM's pattern
	// (UCk_SmState_EntityScript::Get_StateTagForClass). Used for debug
	// identification and not for chain matching — chain extension in the
	// unified model reads Plan[0] handles directly.
	UFUNCTION(BlueprintPure,
		Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Get Action Tag For Class")
	static FGameplayTag
	Get_ActionTagForClass(
		TSubclassOf<UCk_GoapAction_EntityScript> InClass);

	UFUNCTION(BlueprintPure,
		Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Get Action Tag")
	FGameplayTag
	Get_ActionTag() const;

	// ----------------------------------------------------------------------------------------------------------------
	// Populated by the builder API, read by FProcessor_Goap_Setup
	// ----------------------------------------------------------------------------------------------------------------

private:
	TArray<ck::goap::FWorldStateCondition_Raw> _Preconditions;
	TArray<ck::goap::FWorldStateEffect_Raw>    _Effects;
	float _Cost = 1.0f;

public:
	friend class ck::FProcessor_Goap_Setup;
	friend class ck::FProcessor_Goap_Action_Setup;
};

// --------------------------------------------------------------------------------------------------------------------
