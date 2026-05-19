#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"
#include "CkGoap/Algorithm/CkGoap_Graph.h"

#include "CkAStar/CkAStar_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decl in global scope so the `friend class UCk_Utils_Goap_UE;` lines below resolve to the
// real global class — without this, the friend lookup binds to ck::UCk_Utils_Goap_UE (the enclosing
// namespace) and silently grants access to the wrong (non-existent) class. Mirrors the pattern in
// CkInventory_Fragment.h. Keep this above the `namespace ck` block.
class UCk_Utils_Goap_UE;

// ====================================================================================================================
//
// NOTE: FFragment_RecordOfGoapPlanners (the owner's record of GOAP children created by
// Utils::Create) is intentionally NOT declared here. Defining it would force this public header
// to include CkRecord_Fragment.h, which transitively pulls FCk_Handle_EntityExtension into every
// consumer of CkGoap. That trips link errors in dependents (e.g. CkGoapDebugger) that don't list
// CkEntityExtension in their Build.cs. The record fragment + its nested utils struct live in
// CkGoap_Utils.cpp; only Utils.cpp ever touches them. Processors don't need them.
//
// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// TAGS
// ====================================================================================================================

CK_DEFINE_ECS_TAG(FTag_Goap_RequiresSetup);
CK_DEFINE_ECS_TAG(FTag_Goap_PlanRequested);

// Dirty-tracking tags. Added by request handlers on actual value change (not
// on every Set call). Consumed + removed by FProcessor_Goap_AutoReplan once
// the entity's replan policy + throttle permit firing a Plan request.
CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_WorldState);
CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_Cost);

// Drives PlanOnStart. Added by Add() when params opt in; removed by AutoReplan
// after it fires the initial Plan request (post-Setup).
CK_DEFINE_ECS_TAG(FTag_Goap_RequiresInitialPlan);

// ====================================================================================================================
// PARAMS — user-configurable options (policy, budgets, startup behavior, WS source)
// ====================================================================================================================

using FFragment_Goap_Params = FCk_Fragment_Goap_ParamsData;

// Bundle/Tier era: alias for the (currently-empty) root-container params.
// Lives on the FCk_Handle_Goap entity, alongside FFragment_RecordOfGoapBundles
// (defined in CkGoap_Utils.cpp once Phase 3 introduces the new Add verb).
using FFragment_Goap_Root_Params = FCk_Fragment_Goap_RootParamsData;

// ====================================================================================================================
// ACTION CLASSES FRAGMENT — Registered action EntityScript classes (set during Add)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_ActionClasses
{
public:
	CK_GENERATED_BODY(FFragment_Goap_ActionClasses);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;

private:
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Classes;

public:
	CK_PROPERTY_GET(_Classes);
};

// ====================================================================================================================
// ACTIONS FRAGMENT — Extracted lightweight action data (populated by setup processor from CDOs)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Actions
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Actions);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	TArray<goap::FActionDef> _ActionDefs;

public:
	CK_PROPERTY_GET(_ActionDefs);
};

// ====================================================================================================================
// GOAL CLASSES FRAGMENT — Registered goal EntityScript classes (set during Add)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_GoalClasses
{
public:
	CK_GENERATED_BODY(FFragment_Goap_GoalClasses);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_Setup;

private:
	TArray<TSubclassOf<UCk_GoapGoal_EntityScript>> _Classes;

public:
	CK_PROPERTY_GET(_Classes);
};

// ====================================================================================================================
// GOALS FRAGMENT — Extracted lightweight goal data (populated by setup processor from CDOs)
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Goals
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Goals);

	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	TArray<goap::FGoalDef> _GoalDefs;

public:
	CK_PROPERTY_GET(_GoalDefs);
};

// ====================================================================================================================
// CURRENT FRAGMENT — Plan status, active goal, plan result
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Current
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Current);

	friend class FProcessor_Goap_HandleRequests;
	friend class FProcessor_Goap_HandleResult;

private:
	ECk_GoapPlanStatus _PlanStatus = ECk_GoapPlanStatus::Idle;
	TSubclassOf<UCk_GoapGoal_EntityScript> _ActiveGoalClass;
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Plan;
	float _PlanCost = 0.0f;

	// Monotonic counter — incremented on every Request_Plan. Debug tooling uses
	// it to detect "a new plan attempt happened" without relying on observing
	// the transient Planning state (which can be overwritten within one frame
	// when the reachability short-circuit fires).
	int32 _PlanAttemptCount = 0;

public:
	CK_PROPERTY_GET(_PlanStatus);
	CK_PROPERTY_GET(_ActiveGoalClass);
	CK_PROPERTY_GET(_Plan);
	CK_PROPERTY_GET(_PlanCost);
	CK_PROPERTY_GET(_PlanAttemptCount);
};

// ====================================================================================================================
// REQUESTS FRAGMENT — Request queue
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Requests
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Requests);

	friend class UCk_Utils_Goap_UE;
	friend class FProcessor_Goap_HandleRequests;
	friend class FProcessor_Goap_AutoReplan;

	using RequestType = std::variant<
		FCk_Request_Goap_Plan,
		FCk_Request_Goap_CancelPlan,
		FCk_Request_Goap_SetReplanInterval,
		FCk_Request_Goap_SetReplanPolicy,
		FCk_Request_Goap_SetSearchBudget,
		FCk_Request_Goap_SetCostThreshold,
		FCk_Request_Goap_SetActionCost>;

private:
	TArray<RequestType> _Requests;

public:
	CK_PROPERTY_GET(_Requests);
};

// ====================================================================================================================
// REPLAN THROTTLE FRAGMENT — time accumulator for MinReplanIntervalSeconds
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_ReplanThrottle
{
public:
	CK_GENERATED_BODY(FFragment_Goap_ReplanThrottle);

	friend class FProcessor_Goap_AutoReplan;

private:
	// Accumulates DeltaT every tick; reset to 0 whenever a replan fires.
	float _SecondsSinceLastReplan = 0.0f;

public:
	CK_PROPERTY_GET(_SecondsSinceLastReplan);
};

// ====================================================================================================================
// DIAGNOSTICS FRAGMENT — Setup-time graph analysis + plan-time reachability failures
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_Diagnostics
{
public:
	CK_GENERATED_BODY(FFragment_Goap_Diagnostics);

	friend class FProcessor_Goap_Setup;
	friend class FProcessor_Goap_HandleRequests;

private:
	// Static cycles detected once at setup time (post-CDO-extraction). Stable
	// across plans — only invalidated when actions are re-registered.
	TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

	// Populated whenever the last Request_Plan short-circuited or failed due
	// to goal conditions that aren't reachable from the current world state.
	TArray<FCk_GoapDiagnostic_ConditionPair> _LastUnreachableGoalConditions;

	// The goal class tied to the last reachability failure, if any.
	TSubclassOf<UCk_GoapGoal_EntityScript> _LastFailedGoalClass;

public:
	CK_PROPERTY_GET(_DependencyCycles);
	CK_PROPERTY_GET(_LastUnreachableGoalConditions);
	CK_PROPERTY_GET(_LastFailedGoalClass);
};

// ====================================================================================================================
// PLAN CONTEXT FRAGMENT — Holds graph reference for plan extraction after search completes
// ====================================================================================================================

struct CKGOAP_API FFragment_Goap_PlanContext
{
public:
	CK_GENERATED_BODY(FFragment_Goap_PlanContext);

	friend class FProcessor_Goap_HandleRequests;
	friend class FProcessor_Goap_HandleResult;

private:
	goap::FGoapGraph _Graph;

public:
	CK_PROPERTY_GET(_Graph);
};

// ====================================================================================================================
// TEMPLATE INSTANTIATIONS — Concrete A* fragment types for GOAP
// ====================================================================================================================

using FFragment_Goap_SearchState = TFragment_AStar_SearchState<int32, goap::FGoapGraph>;
using FFragment_Goap_Result = TFragment_AStar_Result<int32>;

// ====================================================================================================================
// SIGNALS
// ====================================================================================================================

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapPlanComplete,
	FCk_Delegate_Goap_OnPlanComplete,
	FCk_Handle_Goap,
	FCk_Goap_Payload_OnPlanComplete);

CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
	CKGOAP_API,
	OnGoapPlanFailed,
	FCk_Delegate_Goap_OnPlanFailed,
	FCk_Handle_Goap,
	FCk_Goap_Payload_OnPlanFailed);

// ====================================================================================================================

} // namespace ck
