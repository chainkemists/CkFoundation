#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"

#include "CkGoapGoal_EntityScript.generated.h"

// ====================================================================================================================

namespace ck { class FProcessor_Goap_Setup; }

// ====================================================================================================================

// GOAP Goal definition class.
// Users subclass this to define goal conditions and priority.
// The system reads CDOs — no per-entity instances are created during planning.
//
// Usage (C++):
//   class UMyKillEnemyGoal : public UCk_GoapGoal_EntityScript
//   {
//       auto DefineGoal() -> void override
//       {
//           AddCondition(Tag_EnemyAlive, false);
//           SetPriority(10);
//       }
//   };

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGOAP_API UCk_GoapGoal_EntityScript : public UObject
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_GoapGoal_EntityScript);

	// ================================================================================================================
	// VIRTUAL — Override to define goal metadata
	// ================================================================================================================

public:
	virtual auto
	DefineGoal() -> void;

	// ================================================================================================================
	// BLUEPRINT IMPLEMENTABLE
	// ================================================================================================================

protected:
	UFUNCTION(BlueprintImplementableEvent,
		Category = "Ck|GOAP|Goal",
		DisplayName = "Define Goal")
	void
	DoDefineGoal();

	// ================================================================================================================
	// BUILDER API — Call from DefineGoal
	// ================================================================================================================

protected:
	UFUNCTION(BlueprintCallable,
		Category = "Ck|GOAP|Goal",
		DisplayName = "[Ck][GOAP] Add Condition")
	void
	AddCondition(
		FGameplayTag InKey,
		bool InValue);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|GOAP|Goal",
		DisplayName = "[Ck][GOAP] Set Priority")
	void
	SetPriority(
		int32 InPriority);

	// ================================================================================================================
	// DATA — Populated by builder API, read by FProcessor_Goap_Setup
	// ================================================================================================================

private:
	ck::goap::FWorldState _Conditions;
	int32 _Priority = 0;

	friend class ck::FProcessor_Goap_Setup;
};

// ====================================================================================================================
