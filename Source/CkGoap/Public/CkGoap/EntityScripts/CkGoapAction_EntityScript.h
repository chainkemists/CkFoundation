#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkGoapAction_EntityScript.generated.h"

// ====================================================================================================================

namespace ck { class FProcessor_Goap_Setup; }

// ====================================================================================================================
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

// ====================================================================================================================

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
	// BUILDERS
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

	// Identifies this action for the Bundle/Tier chain-update rule. When
	// Plan[0]._ActionTag matches some Tier._TierTag in the same bundle, that
	// tier auto-activates as the planning tier's child. Leave unset for "leaf"
	// actions that never trigger a sub-tier.
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Set Action Tag")
	void
	SetActionTag(FGameplayTag InTag);

	// ----------------------------------------------------------------------------------------------------------------
	// DATA — populated by the builder API, read by FProcessor_Goap_Setup
	// ----------------------------------------------------------------------------------------------------------------

private:
	TArray<ck::goap::FWorldStateCondition_Raw> _Preconditions;
	TArray<ck::goap::FWorldStateEffect_Raw>    _Effects;
	float _Cost = 1.0f;
	FGameplayTag _ActionTag;

public:
	auto Get_ActionTag() const -> FGameplayTag { return _ActionTag; }

	friend class ck::FProcessor_Goap_Setup;
};

// ====================================================================================================================
