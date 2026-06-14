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

	// AddAction — register a child Action under this Planner.
	//
	// Semantics depend on whether the Planner is top-level or a promoted mid-tier:
	//
	// * Top-level Planner (no Action role on the host entity): the first
	//   AddAction call creates the implicit-root Action — the entity that
	//   actually runs A* for this Planner. Subsequent AddAction calls become
	//   tree children of that implicit root (they are the planner's candidate
	//   operators). WS source for the implicit root falls back to the
	//   Planner's _WorldStateSource (set on PlannerParams) when the Action's
	//   own _WorldStateSource_Override is unset.
	//
	// * Promoted mid-tier Planner (host entity carries the Action role): every
	//   AddAction call registers a direct tree child of the host. The host
	//   itself is the Action that runs A*; its children are the candidate
	//   operators consumed by the planner. New children inherit the host's
	//   resolved WS at activation time (no implicit-root indirection).
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Add Action")
	static FCk_Handle_Goap_Action
	AddAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams);

	// U11.3: Promote an existing Action entity to be ALSO a Planner.
	//
	// Path A (transitional): every Action entity already carries the planner-role
	// fragment cluster (PlanState, Goal, WorldStateSource, Activation) — the
	// only fragments missing from an Action to act as a Planner are the Planner
	// *identity / discriminator* fragments (Params + Current + ActionCatalogIndex).
	// Promotion stamps those, copies InParams values onto the entity, and
	// returns the Planner-cast handle.
	//
	// After promotion, both casts succeed on the same entity:
	//   * UCk_Utils_Goap_Action_UE::Cast(handle) → action handle (preserved)
	//   * UCk_Utils_Goap_Planner_UE::Cast(handle) → planner handle (new role)
	//
	// The promoted Planner has its own goal (independent of the Action role's
	// effects) and can have children added under it via subsequent AddAction
	// calls passing the Planner-cast handle.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Promote Action To Planner")
	static FCk_Handle_Goap_Planner
	PromoteActionToPlanner(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

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
		DisplayName = "[Ck][Goap|Planner] Get Plan Status")
	static ECk_GoapPlanStatus
	Get_PlanStatus(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Plan")
	static TArray<FCk_Handle_Goap_Action>
	Get_Plan(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Plan Classes")
	static TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
	Get_PlanClasses(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Plan Cost")
	static float
	Get_PlanCost(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Plan Attempt Count")
	static int32
	Get_PlanAttemptCount(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get World State Source (Resolved)")
	static FCk_Handle_Goap_WorldState
	Get_WorldStateSource(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Invalid Goal")
	static TArray<FCk_GoapWS_Condition_Authored>
	Get_InvalidGoal(const FCk_Handle_Goap_Planner& InPlanner);

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
		DisplayName = "[Ck][Goap|Planner] Request Reset Active Chain")
	static FCk_Handle_Goap_Planner
	Request_ResetActiveChain(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner);

	// U11.1: set the Planner's goal at runtime. Triggers a replan. Per spec
	// §3.3 — every Planner has its own goal, completely independent from any
	// Action-role effects this entity may carry. The request is routed onto
	// the Planner's root Action's request queue (the entity that runs A*
	// today); the fragment rename to FFragment_Goap_Planner_Requests is U11.5.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Goal")
	static FCk_Handle_Goap_Planner
	Request_SetGoal(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal);

	// Planner-API request verbs. Enqueue on the Planner's own request queue.

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Plan")
	static FCk_Handle_Goap_Planner
	Request_Plan(UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Cancel Plan")
	static FCk_Handle_Goap_Planner
	Request_CancelPlan(UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Replan Interval")
	static FCk_Handle_Goap_Planner
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		float InSeconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Replan Policy")
	static FCk_Handle_Goap_Planner
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		ECk_Goap_ReplanPolicy InPolicy);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Search Budget")
	static FCk_Handle_Goap_Planner
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		int64 InMicroseconds);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Cost Threshold")
	static FCk_Handle_Goap_Planner
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		float InThreshold);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Child Action Cost")
	static FCk_Handle_Goap_Planner
	Request_SetChildActionCost(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		float InCost);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Register Action Cost Provider")
	static FCk_Handle_Goap_Planner
	Request_RegisterActionCostProvider(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass);

	// Request_RemoveAction — runtime catalog mutation: remove a previously-
	// registered child Action from this Planner. Inverse of AddAction.
	//
	// Steps performed:
	//   1. Resolve the child Action handle via the class-derived tag (catalog
	//      lookup — same path AddAction uses).
	//   2. Remove the catalog entry from _TagToAction.
	//   3. Remove the handle from the parent's _ChildActions tree (parent =
	//      the Action's recorded _ParentAction — implicit root for top-level
	//      Planners, the promoted host for mid-tier Planners).
	//   4. Destroy the Action entity via the standard entity-lifetime path.
	//      The record-of-actions entry is removed by the entity's EndPlay
	//      cleanup (owner-cascade-destroy).
	//   5. Mark the Planner for re-setup (cycle detection / catalog rebuild)
	//      and enqueue a Request_Plan so the next plan reflects the updated
	//      operator set.
	//
	// If the class is not registered on this Planner, emits a warning and
	// returns the Planner unchanged.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Remove Action")
	static FCk_Handle_Goap_Planner
	Request_RemoveAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildActionClass);

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

	// PR-B.1b Stage 0 — per-Planner signals (spec §3.5). Bind/Unbind resolve
	// the Planner to the entity that actually broadcasts under Path A (the
	// implicit-root Action for top-level Planners; the host entity itself for
	// promoted mid-tier Planners) and bind delegates there. Stage 3 will move
	// the broadcast onto the Planner entity itself and drop the resolution
	// step.

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Bind To OnPlanComplete")
	static FCk_Handle_Goap_Planner
	BindTo_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Unbind From OnPlanComplete")
	static FCk_Handle_Goap_Planner
	UnbindFrom_OnPlanComplete(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Bind To OnPlanFailed")
	static FCk_Handle_Goap_Planner
	BindTo_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Unbind From OnPlanFailed")
	static FCk_Handle_Goap_Planner
	UnbindFrom_OnPlanFailed(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Bind To OnPlannerActivated")
	static FCk_Handle_Goap_Planner
	BindTo_OnPlannerActivated(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Unbind From OnPlannerActivated")
	static FCk_Handle_Goap_Planner
	UnbindFrom_OnPlannerActivated(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Bind To OnPlannerDeactivated")
	static FCk_Handle_Goap_Planner
	BindTo_OnPlannerDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Unbind From OnPlannerDeactivated")
	static FCk_Handle_Goap_Planner
	UnbindFrom_OnPlannerDeactivated(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate);
};
