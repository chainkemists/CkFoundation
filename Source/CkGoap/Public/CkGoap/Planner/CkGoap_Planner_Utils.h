#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"   // FCk_Fragment_Goap_ActionParamsData, FCk_Handle_Goap_Action
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"     // UCk_GoapAction_EntityScript
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Planner_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Planner"))
class CKGOAP_API UCk_Utils_Goap_Planner_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Planner_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Planner);

public:
	// ================================================================================================================
	// CONSTRUCTION
	// ================================================================================================================

	// Add — spawn a child Planner entity off InOwner, label it with the Params
	// tag, and register it in the Owner's record of Planners. In U11.0a the
	// PlannerParams' tag is required (same identity as the old ActionSet tag).
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Add")
	static FCk_Handle_Goap_Planner
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

	// Create — convenience that takes the tag separately. Writes the tag into a
	// copy of InParams then calls Add.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Create")
	static FCk_Handle_Goap_Planner
	Create(
		UPARAM(ref) FCk_Handle& InOwner,
		FGameplayTag InPlannerTag,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

	// Find_Planner — lookup a named planner on the Owner.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Find Planner")
	static FCk_Handle_Goap_Planner
	Find_Planner(
		const FCk_Handle& InOwner,
		FGameplayTag InPlannerTag);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Set Root Action")
	static FCk_Handle_Goap_Action
	SetRootAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		UPARAM(ref) FCk_Handle_Goap_WorldState& InInitialWorldState);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Add Action To ActionSet")
	static FCk_Handle_Goap_Action
	AddAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Has")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Find Action")
	static FCk_Handle_Goap_Action
	Find_Action(
		const FCk_Handle_Goap_Planner& InPlanner,
		FGameplayTag InActionTag);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Find Action By Class")
	static FCk_Handle_Goap_Action
	Find_ActionByClass(
		const FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Active Chain")
	static TArray<FCk_Handle_Goap_Action>
	Get_ActiveChain(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Enable Toggle")
	static ECk_EnableDisable
	Get_EnableToggle(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Dependency Cycles")
	static TArray<FCk_GoapDiagnostic_DependencyCycle>
	Get_DependencyCycles(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Root Action")
	static FCk_Handle_Goap_Action
	Get_RootAction(const FCk_Handle_Goap_Planner& InPlanner);

	// ================================================================================================================
	// REQUESTS
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Enable Toggle")
	static FCk_Handle_Goap_Planner
	Request_SetEnableToggle(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		ECk_EnableDisable InToggle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Root Action")
	static FCk_Handle_Goap_Planner
	Request_SetRootAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		UPARAM(ref) FCk_Handle_Goap_WorldState& InInitialWorldState);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Reset Active Chain")
	static FCk_Handle_Goap_Planner
	Request_ResetActiveChain(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Bind To OnActiveChainChanged")
	static FCk_Handle_Goap_Planner
	BindTo_OnActiveChainChanged(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Unbind From OnActiveChainChanged")
	static FCk_Handle_Goap_Planner
	UnbindFrom_OnActiveChainChanged(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate);
};
