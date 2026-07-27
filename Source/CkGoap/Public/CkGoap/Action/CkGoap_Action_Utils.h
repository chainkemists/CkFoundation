#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Action_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Action"))
class CKGOAP_API UCk_Utils_Goap_Action_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Action_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Action);

public:
// --------------------------------------------------------------------------------------------------------------------

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

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Has Cost Provider")
	static bool
	Get_HasCostProvider(const FCk_Handle_Goap_Action& InAction);

	/**
	 * True once FProcessor_Goap_Action_Setup has extracted this Action's CDO
	 * definition (preconditions/effects/cost) into its cached operator def.
	 * A Request_Plan issued before this returns true sees a default-constructed
	 * candidate and silently ignores the new Action — wait on this after a
	 * runtime AddAction before forcing a replan.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Get Is Setup Complete")
	static bool
	Get_IsSetupComplete(const FCk_Handle_Goap_Action& InAction);

// --------------------------------------------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Plan",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_Plan(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Cancel Plan",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_CancelPlan(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Action Cost",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_SetActionCost(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InCost,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Replan Interval",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		float InSeconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Replan Policy",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		ECk_Goap_ReplanPolicy InPolicy,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Search Budget",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		int64 InMicroseconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Action",
		DisplayName = "[Ck][Goap|Action] Request Set Cost Threshold",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Action
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		float InThreshold,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

};
