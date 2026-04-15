#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"

#include "CkGoapAction_EntityScript.generated.h"

// ====================================================================================================================

namespace ck { class FProcessor_Goap_Setup; }

// ====================================================================================================================

// GOAP Action definition class.
// Users subclass this to define action metadata (preconditions, effects, cost).
// The system reads CDOs — no per-entity instances are created during planning.
//
// Usage (C++):
//   class UMyAttackAction : public UCk_GoapAction_EntityScript
//   {
//       auto DefineAction() -> void override
//       {
//           AddPrecondition(Tag_HasWeapon, true);
//           AddPrecondition(Tag_EnemyInRange, true);
//           AddEffect(Tag_EnemyAlive, false);
//           SetCost(3.0f);
//       }
//   };
//
// Usage (Angelscript/Blueprint):
//   Override DoDefineAction, call DoAddPrecondition / DoAddEffect / DoSetCost.

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGOAP_API UCk_GoapAction_EntityScript : public UObject
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_GoapAction_EntityScript);

	// ================================================================================================================
	// VIRTUAL — Override to define action metadata
	// ================================================================================================================

public:
	virtual auto
	DefineAction() -> void;

	// ================================================================================================================
	// BLUEPRINT IMPLEMENTABLE
	// ================================================================================================================

protected:
	UFUNCTION(BlueprintImplementableEvent,
		Category = "Ck|GOAP|Action",
		DisplayName = "Define Action")
	void
	DoDefineAction();

	// ================================================================================================================
	// BUILDER API — Call from DefineAction
	// ================================================================================================================

protected:
	UFUNCTION(BlueprintCallable,
		Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Add Precondition")
	void
	AddPrecondition(
		FGameplayTag InKey,
		bool InValue);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Add Effect")
	void
	AddEffect(
		FGameplayTag InKey,
		bool InValue);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|GOAP|Action",
		DisplayName = "[Ck][GOAP] Set Cost")
	void
	SetCost(
		float InCost);

	// ================================================================================================================
	// DATA — Populated by builder API, read by FProcessor_Goap_Setup
	// ================================================================================================================

private:
	ck::goap::FWorldState _Preconditions;
	ck::goap::FWorldState _Effects;
	float _Cost = 1.0f;

	friend class ck::FProcessor_Goap_Setup;
};

// ====================================================================================================================
