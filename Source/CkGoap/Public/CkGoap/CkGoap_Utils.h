#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Utils.generated.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;
class UCk_GoapGoal_EntityScript;

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap"))
class CKGOAP_API UCk_Utils_Goap_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap);

public:
	// ================================================================================================================
	// CREATION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Add GOAP Planner")
	static FCk_Handle_Goap
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		const FCk_Fragment_Goap_ParamsData& InParams);

	// ================================================================================================================
	// ACTION & GOAL REGISTRATION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Add Action")
	static FCk_Handle_Goap
	AddAction(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Add Goal")
	static FCk_Handle_Goap
	AddGoal(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass);

	// ================================================================================================================
	// WORLD STATE — TYPED
	// ================================================================================================================
	//
	// Keys must be registered before use. A key is registered when it appears
	// in any action's precondition/effect or any goal's condition — the Setup
	// processor scans all CDOs and populates the key registry once. Setting
	// a value on a tag that wasn't registered is a silent no-op (warned in
	// Verbose logs).

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Set World State Value")
	static FCk_Handle_Goap
	Set_WorldStateValue(
		UPARAM(ref) FCk_Handle_Goap& InGoap, FGameplayTag InKey, bool InValue);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get World State Value")
	static bool
	Get_WorldStateValue(const FCk_Handle_Goap& InGoap, FGameplayTag InKey);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Has World State Key")
	static bool
	Has_WorldStateKey(const FCk_Handle_Goap& InGoap, FGameplayTag InKey);

	// ================================================================================================================
	// DYNAMIC COST UPDATE
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Set Action Cost")
	static FCk_Handle_Goap
	Set_ActionCost(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InNewCost);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get Action Cost")
	static float
	Get_ActionCost(
		const FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass);

	// ================================================================================================================
	// PLANNING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Plan")
	static FCk_Handle_Goap
	Request_Plan(UPARAM(ref) FCk_Handle_Goap& InGoap);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Plan For Goal")
	static FCk_Handle_Goap
	Request_PlanForGoal(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Cancel Plan")
	static FCk_Handle_Goap
	Request_CancelPlan(UPARAM(ref) FCk_Handle_Goap& InGoap);

	// ================================================================================================================
	// RUNTIME TUNING (via request queue)
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Set Replan Interval")
	static FCk_Handle_Goap
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		float InMinReplanIntervalSeconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Set Replan Policy")
	static FCk_Handle_Goap
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		ECk_Goap_ReplanPolicy InPolicy);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Set Search Budget (microseconds)")
	static FCk_Handle_Goap
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		int64 InSearchBudgetMicroseconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Request Set Cost Threshold")
	static FCk_Handle_Goap
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		float InCostThreshold);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Has Feature")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get Plan Status")
	static ECk_GoapPlanStatus
	Get_PlanStatus(const FCk_Handle_Goap& InGoap);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get Plan")
	static TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
	Get_Plan(const FCk_Handle_Goap& InGoap);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get Plan Cost")
	static float
	Get_PlanCost(const FCk_Handle_Goap& InGoap);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Bind To OnPlanComplete")
	static FCk_Handle_Goap
	BindTo_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Unbind From OnPlanComplete")
	static FCk_Handle_Goap
	UnbindFrom_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Bind To OnPlanFailed")
	static FCk_Handle_Goap
	BindTo_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Unbind From OnPlanFailed")
	static FCk_Handle_Goap
	UnbindFrom_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate);

	// ================================================================================================================
	// DIAGNOSTICS
	// ================================================================================================================

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|Diagnostics",
		DisplayName = "[Ck][GOAP] Get Dependency Cycles")
	static TArray<FCk_GoapDiagnostic_DependencyCycle>
	Get_DependencyCycles(const FCk_Handle_Goap& InGoap);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|Diagnostics",
		DisplayName = "[Ck][GOAP] Get Unreachable Goal Conditions (last plan)")
	static TArray<FCk_GoapDiagnostic_ConditionPair>
	Get_LastUnreachableGoalConditions(const FCk_Handle_Goap& InGoap);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|Diagnostics",
		DisplayName = "[Ck][GOAP] Has Diagnostic Warnings")
	static bool
	Has_DiagnosticWarnings(const FCk_Handle_Goap& InGoap);

	// ================================================================================================================
	// CAST
	// ================================================================================================================

private:
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Cast",
		meta = (ExpandEnumAsExecs = "OutResult"))
	static FCk_Handle_Goap
	DoCast(
		UPARAM(ref) FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Handle -> GOAP Handle",
		meta = (CompactNodeTitle = "<AsGOAP>", BlueprintAutocast))
	static FCk_Handle_Goap
	DoCastChecked(FCk_Handle InHandle);

	UFUNCTION(BlueprintPure,
		DisplayName = "[Ck] Get Invalid GOAP Handle",
		Category = "Ck|GOAP",
		meta = (CompactNodeTitle = "INVALID_GoapHandle", Keywords = "make"))
	static FCk_Handle_Goap
	Get_InvalidHandle() { return {}; }

private:
	static auto
	DoAddRequest(FCk_Handle_Goap& InGoap, const auto& InRequest) -> FCk_Handle_Goap;
};

// ====================================================================================================================
