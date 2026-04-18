#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoapAction_EntityScript.h"

#include "CkGoapGoal_EntityScript.generated.h"

// ====================================================================================================================

namespace ck { class FProcessor_Goap_Setup; }

// ====================================================================================================================

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKGOAP_API UCk_GoapGoal_EntityScript : public UObject
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_GoapGoal_EntityScript);

public:
	virtual auto
	DefineGoal() -> void;

protected:
	UFUNCTION(BlueprintImplementableEvent,
		Category = "Ck|GOAP|Goal",
		DisplayName = "Define Goal")
	void
	DoDefineGoal();

	// ----------------------------------------------------------------------------------------------------------------
	// BUILDERS
	// ----------------------------------------------------------------------------------------------------------------

protected:
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Goal",
		DisplayName = "[Ck][GOAP] Add Condition")
	void
	AddCondition(FGameplayTag InKey, bool InValue);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|Goal",
		DisplayName = "[Ck][GOAP] Set Priority")
	void
	SetPriority(int32 InPriority);

	// ----------------------------------------------------------------------------------------------------------------
	// DATA
	// ----------------------------------------------------------------------------------------------------------------

private:
	TArray<ck::goap::FWorldStateCondition_Raw> _Conditions;
	int32 _Priority = 0;

	friend class ck::FProcessor_Goap_Setup;
};

// ====================================================================================================================
