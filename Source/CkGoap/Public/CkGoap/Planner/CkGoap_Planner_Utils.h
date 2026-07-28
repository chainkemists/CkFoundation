#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Planner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Planner"))
class CKGOAP_API UCk_Utils_Goap_Planner_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Planner_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Planner);

public:
// --------------------------------------------------------------------------------------------------------------------

	// Stamps the Planner role onto InOwner itself — no child entity, no GameplayLabel. Returns an
	// invalid handle if InOwner already carries the Planner role; use Create for a second one.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Add")
	static FCk_Handle_Goap_Planner
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

	// Spawns a NEW child Planner entity off InOwner, labelled and recorded under InPlannerTag (the
	// entry's identity, also written into the params copy). For multiple Planners on one owner.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Create")
	static FCk_Handle_Goap_Planner
	Create(
		UPARAM(ref) FCk_Handle& InOwner,
		FGameplayTag InPlannerTag,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

	// Resolves Create-spawned child Planners only — an Add-stamped Planner is not recorded.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Find Planner")
	static FCk_Handle_Goap_Planner
	Find_Planner(
		const FCk_Handle& InOwner,
		FGameplayTag InPlannerTag);

	// Registers a direct child Action of this Planner — a candidate operator in its catalog.
	// Re-adding a class already in the catalog warns and returns the existing handle.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Add Action")
	static FCk_Handle_Goap_Action
	AddAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams);

	// Adds the Planner role to an existing Action entity; both casts then succeed on it. The
	// promoted Planner's own goal is independent of the Action role's effects.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Promote Action To Planner")
	static FCk_Handle_Goap_Planner
	PromoteActionToPlanner(
		UPARAM(ref) FCk_Handle_Goap_Action& InAction,
		const FCk_Fragment_Goap_PlannerParamsData& InParams);

// --------------------------------------------------------------------------------------------------------------------

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

	// ---- Planner-params reads (the debugger's Settings drawer) ---------------

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Planner Tag")
	static FGameplayTag
	Get_PlannerTag(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Replan Policy")
	static ECk_Goap_ReplanPolicy
	Get_ReplanPolicy(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Min Replan Interval")
	static float
	Get_MinReplanInterval(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Search Budget Microseconds")
	static int64
	Get_SearchBudgetMicroseconds(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Cost Threshold")
	static float
	Get_CostThreshold(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Plan On Start")
	static bool
	Get_PlanOnStart(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Allow Plan Failed")
	static bool
	Get_AllowPlanFailed(const FCk_Handle_Goap_Planner& InPlanner);

	// True iff some registered Action has no preconditions AND effects covering every goal
	// condition — the always-valid-plan tenet, checked once at Setup.
	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Has Unconditional Fallback")
	static bool
	Get_HasUnconditionalFallback(const FCk_Handle_Goap_Planner& InPlanner);

	// ---- Last-replan diagnostics (the debugger's timeline + diff card) -------

	// Why the last replan fired. ChangedKeys.Num() > 1 means the throttle window coalesced several.
	// Default-constructed if this Planner has never planned.
	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Last Replan Cause")
	static FCk_Goap_ReplanCauseInfo
	Get_LastReplanCause(const FCk_Handle_Goap_Planner& InPlanner);

	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Last Search Stats")
	static FCk_Goap_SearchStats
	Get_LastSearchStats(const FCk_Handle_Goap_Planner& InPlanner);

	// One row per constraint set discovered by the last regressive search, in discovery order —
	// row 0 is the goal. Empty before the first search.
	UFUNCTION(BlueprintPure, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Get Last Search Debug")
	static TArray<FCk_Goap_SearchDebugRow>
	Get_LastSearchDebug(const FCk_Handle_Goap_Planner& InPlanner);

// --------------------------------------------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Enable Toggle",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetEnableToggle(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		ECk_EnableDisable InToggle,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Reset Active Chain",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_ResetActiveChain(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	// Triggers a replan. This goal is independent of any Action-role effects the entity carries.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Goal",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetGoal(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Plan",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_Plan(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Cancel Plan",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_CancelPlan(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Replan Interval",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetReplanInterval(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		float InSeconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Replan Policy",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetReplanPolicy(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		ECk_Goap_ReplanPolicy InPolicy,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Search Budget",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetSearchBudget(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		int64 InMicroseconds,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Cost Threshold",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetCostThreshold(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		float InThreshold,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Set Child Action Cost",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_SetChildActionCost(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		float InCost,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Register Action Cost Provider",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_RegisterActionCostProvider(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	// Inverse of AddAction: unregisters the child Action, destroys its entity, and replans. Warns
	// and leaves the Planner unchanged when the class is not registered on it.
	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Planner",
		DisplayName = "[Ck][Goap|Planner] Request Remove Action",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_Planner
	Request_RemoveAction(
		UPARAM(ref) FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildActionClass,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

// --------------------------------------------------------------------------------------------------------------------

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

private:
	// A private static member rather than a free helper: direct `_Member` writes need the friend
	// access the Planner-role fragments grant this class.
	static auto
	DoStampTopLevelPlannerRole(
		FCk_Handle& InPlannerEntity,
		const FCk_Fragment_Goap_PlannerParamsData& InParams) -> void;
};
