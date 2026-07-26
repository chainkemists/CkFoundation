#pragma once

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkGoapAction_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Goap_Setup; class FProcessor_Goap_Action_Setup; }

// --------------------------------------------------------------------------------------------------------------------
// Subclass and override DefineAction (C++) or DoDefineAction (BP / AS); the builder
// API records (tag, bool) preconditions and effects on the CDO, which Setup reads
// once per entity. Authoring guide: CkGoap CLAUDE.md.

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
	// Identity tag derived from the class name. Debug identification only — chain
	// extension matches on Plan[0] handles, not on this tag.
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
