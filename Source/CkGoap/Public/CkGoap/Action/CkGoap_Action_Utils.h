#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Action_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Action"))
class CKGOAP_API UCk_Utils_Goap_Action_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Action_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Action);

public:
	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Has")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Plan Status")
	static ECk_GoapPlanStatus
	Get_PlanStatus(const FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Plan")
	static TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
	Get_Plan(const FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Plan Cost")
	static float
	Get_PlanCost(const FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get World State Source (Resolved)")
	static FCk_Handle_Goap_WorldState
	Get_WorldStateSource(const FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Active Parent Action")
	static TSubclassOf<UCk_GoapAction_EntityScript>
	Get_ActiveParentAction(const FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Invalid Goal")
	static TArray<FCk_GoapWS_Condition_Authored>
	Get_InvalidGoal(const FCk_Handle_Goap_Action& InAction);

	// ================================================================================================================
	// REQUESTS
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Plan")
	static FCk_Handle_Goap_Action
	Request_Plan(UPARAM(ref) FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Cancel Plan")
	static FCk_Handle_Goap_Action
	Request_CancelPlan(UPARAM(ref) FCk_Handle_Goap_Action& InAction);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Action Cost")
	static FCk_Handle_Goap_Action
	Request_SetActionCost(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InCost);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Replan Interval")
	static FCk_Handle_Goap_Action
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		float InSeconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Replan Policy")
	static FCk_Handle_Goap_Action
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		ECk_Goap_ReplanPolicy InPolicy);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Search Budget")
	static FCk_Handle_Goap_Action
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		int64 InMicroseconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Cost Threshold")
	static FCk_Handle_Goap_Action
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		float InThreshold);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Bind To OnPlanComplete")
	static FCk_Handle_Goap_Action
	BindTo_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Unbind From OnPlanComplete")
	static FCk_Handle_Goap_Action
	UnbindFrom_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanComplete& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Bind To OnPlanFailed")
	static FCk_Handle_Goap_Action
	BindTo_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Unbind From OnPlanFailed")
	static FCk_Handle_Goap_Action
	UnbindFrom_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanFailed& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Bind To OnPlannerActivated")
	static FCk_Handle_Goap_Action
	BindTo_OnPlannerActivated(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Unbind From OnPlannerActivated")
	static FCk_Handle_Goap_Action
	UnbindFrom_OnPlannerActivated(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Bind To OnPlannerDeactivated")
	static FCk_Handle_Goap_Action
	BindTo_OnPlannerDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Unbind From OnPlannerDeactivated")
	static FCk_Handle_Goap_Action
	UnbindFrom_OnPlannerDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate);
};
