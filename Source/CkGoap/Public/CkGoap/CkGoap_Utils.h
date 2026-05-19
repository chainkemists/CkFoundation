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
	//
	// Two creation entry points:
	//
	//   Add    — adds the GOAP fragments DIRECTLY to InOwner. The owner entity
	//            *is* the planner. Single planner per owner; calling Add twice
	//            on the same owner is a logical error (ensures and bails). Use
	//            this for the common case where the owner has one planner and
	//            consumers cast InOwner → FCk_Handle_Goap directly. Caller is
	//            responsible for ensuring InOwner does NOT already have a
	//            standalone AStar feature — GOAP stamps its own AStar params
	//            fragment on the planner, which would collide.
	//
	//   Create — creates a NEW child entity owned by InOwner, stamps the GOAP
	//            fragments on the child, applies a GameplayLabel(InName), and
	//            registers it in the owner's RecordOfGoapPlanners. Use this
	//            when an owner needs multiple planners (tactical + strategic),
	//            when the owner already has standalone AStar, or when the
	//            planner's lifetime should be independent of the owner's other
	//            features.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Add GOAP Planner (on Owner)")
	static FCk_Handle_Goap
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		const FCk_Fragment_Goap_ParamsData& InParams);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Create GOAP Planner (named child)")
	static FCk_Handle_Goap
	Create(
		UPARAM(ref) FCk_Handle& InOwner,
		FGameplayTag InName,
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
	// WORLD STATE
	// ================================================================================================================
	//
	// World state lives on the FCk_Handle_Goap_WorldState entity referenced by
	// FCk_Fragment_Goap_ParamsData::_WorldStateSource. Read and write that
	// entity directly via utils_goap_world_state::Set_Value / Get_Value /
	// Has_Key. Use Get_WorldStateSource below to retrieve the WS handle
	// associated with a given planner.

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Get WorldState Source")
	static FCk_Handle_Goap_WorldState
	Get_WorldStateSource(const FCk_Handle_Goap& InGoap);

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

	// Locate the GOAP planner for a given owner. Lookup order:
	//   1. If InHandle itself has the GOAP feature (Add-on-owner case), returns InHandle cast
	//      to FCk_Handle_Goap.
	//   2. Else if InHandle has a RecordOfGoapPlanners (Create case), returns the first valid
	//      entry. For multi-planner owners, prefer Find_GoapByName so the lookup is explicit.
	//   3. Else returns an invalid handle.
	//
	// Use this from polled SM conditions and other consumers that have only the orchestration
	// entity and need to reach the GOAP planner. Avoids the "ck::Ctx returns the SM owner but
	// As_Goap fails because GOAP lives on a child" footgun.
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Find GOAP For Owner")
	static FCk_Handle_Goap
	Find_Goap(const FCk_Handle& InHandle);

	// Look up a GOAP planner child by its GameplayLabel (set by Create). Returns the planner
	// whose label matches InName, or an invalid handle if no match. Use this when an owner
	// has multiple planners created via Create with distinct names.
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP",
		DisplayName = "[Ck][GOAP] Find GOAP By Name")
	static FCk_Handle_Goap
	Find_GoapByName(const FCk_Handle& InHandle, FGameplayTag InName);

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
