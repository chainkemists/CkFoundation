#pragma once

#include "CkGoap/Tier/CkGoap_Tier_Fragment_Data.h"
#include "CkGoap/Bundle/CkGoap_Bundle_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Tier_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Tier"))
class CKGOAP_API UCk_Utils_Goap_Tier_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Tier_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Tier);

public:
	// ================================================================================================================
	// CONSTRUCTION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Add Tier")
	static FCk_Handle_Goap_Tier
	AddTier(
		UPARAM(ref) FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Fragment_Goap_TierParamsData& InParams);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Add Action")
	static FCk_Handle_Goap_Tier
	AddAction(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Has")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get Plan Status")
	static ECk_GoapPlanStatus
	Get_PlanStatus(const FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get Plan")
	static TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
	Get_Plan(const FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get Plan Cost")
	static float
	Get_PlanCost(const FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get World State Source (Resolved)")
	static FCk_Handle_Goap_WorldState
	Get_WorldStateSource(const FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get Active Parent Action")
	static TSubclassOf<UCk_GoapAction_EntityScript>
	Get_ActiveParentAction(const FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Get Invalid Goal")
	static TArray<FCk_GoapWS_Condition_Authored>
	Get_InvalidGoal(const FCk_Handle_Goap_Tier& InTier);

	// ================================================================================================================
	// REQUESTS
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Goal World State")
	static FCk_Handle_Goap_Tier
	Request_SetGoalWorldState(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Plan")
	static FCk_Handle_Goap_Tier
	Request_Plan(UPARAM(ref) FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Cancel Plan")
	static FCk_Handle_Goap_Tier
	Request_CancelPlan(UPARAM(ref) FCk_Handle_Goap_Tier& InTier);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Action Cost")
	static FCk_Handle_Goap_Tier
	Request_SetActionCost(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InCost);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Replan Interval")
	static FCk_Handle_Goap_Tier
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		float InSeconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Replan Policy")
	static FCk_Handle_Goap_Tier
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		ECk_Goap_ReplanPolicy InPolicy);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Search Budget")
	static FCk_Handle_Goap_Tier
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		int64 InMicroseconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Request Set Cost Threshold")
	static FCk_Handle_Goap_Tier
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		float InThreshold);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Bind To OnPlanComplete")
	static FCk_Handle_Goap_Tier
	BindTo_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Unbind From OnPlanComplete")
	static FCk_Handle_Goap_Tier
	UnbindFrom_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanComplete& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Bind To OnPlanFailed")
	static FCk_Handle_Goap_Tier
	BindTo_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Unbind From OnPlanFailed")
	static FCk_Handle_Goap_Tier
	UnbindFrom_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierPlanFailed& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Bind To OnTierActivated")
	static FCk_Handle_Goap_Tier
	BindTo_OnTierActivated(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Unbind From OnTierActivated")
	static FCk_Handle_Goap_Tier
	UnbindFrom_OnTierActivated(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierActivated& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Bind To OnTierDeactivated")
	static FCk_Handle_Goap_Tier
	BindTo_OnTierDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Tier",
		DisplayName = "[Ck][Goap|Tier] Unbind From OnTierDeactivated")
	static FCk_Handle_Goap_Tier
	UnbindFrom_OnTierDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Tier& InTier,
		const FCk_Delegate_Goap_OnTierDeactivated& InDelegate);
};
