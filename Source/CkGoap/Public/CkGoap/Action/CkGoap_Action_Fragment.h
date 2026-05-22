#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"
#include "CkGoap/Algorithm/CkGoap_Graph.h"

#include "CkAStar/CkAStar_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Action_UE;
class UCk_Utils_Goap_Planner_UE;
class UCk_GoapAction_EntityScript;

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_Action_Setup;
	class FProcessor_Goap_Action_HandleRequests;
	class FProcessor_Goap_Action_HandleResult;
	class FProcessor_Goap_Action_AutoReplan;
	class FProcessor_Goap_Planner_ChainUpdate;

// ====================================================================================================================
// TAGS — action-scoped lifecycle
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_Action_RequiresSetup);
	CK_DEFINE_ECS_TAG(FTag_Goap_Action_PlanRequested);

	// Set on action activation; AutoReplan picks it up next frame to fire the
	// first plan request. Removed by AutoReplan once consumed.
	CK_DEFINE_ECS_TAG(FTag_Goap_Action_RequiresInitialPlan);

	// Set on an Action while it is actively planning — added by HandleRequests
	// when a Plan request begins processing (AStar seeded), removed by
	// HandleResult on terminal status (PlanFound / PlanFailed /
	// CostThresholdReached) or by Request_CancelPlan. Used to gate child
	// Actions from draining their own Plan requests while the parent's plan
	// is in flight: a child whose parent has this tag (or whose parent has
	// never produced a plan — status still Idle) defers its Plan request,
	// keeping it queued for retry on the next frame. Root Actions (no parent)
	// are never gated.
	CK_DEFINE_ECS_TAG(FTag_Goap_Action_PlanInFlight);

// ====================================================================================================================
// PARAMS — alias to BlueprintType data
// ====================================================================================================================

	using FFragment_Goap_Action_Params = FCk_Fragment_Goap_ActionParamsData;

// ====================================================================================================================
// CURRENT — residual Action-role state. Post-U11.0 split, _Plan / _PlanCost /
// _PlanStatus / _PlanAttemptCount moved to FFragment_Goap_Planner_PlanState;
// _Goal / _InvalidGoal moved to FFragment_Goap_Planner_Goal;
// _WorldStateSource_Resolved moved to FFragment_Goap_Planner_WorldStateSource
// (as its new _Resolved field). What remains: the "active parent" record —
// the class of the parent Action that injected this Action's current goal.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Current);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		// The action class on the PARENT action that injected this action's
		// current goal. nullptr for the root.
		TSubclassOf<UCk_GoapAction_EntityScript>         _ActiveParentAction;

	public:
		CK_PROPERTY_GET(_ActiveParentAction);
	};

// ====================================================================================================================
// ACTION CLASSES — Registered action EntityScript classes
// (legacy collection — preserved through Phase U1; full removal handled later.)
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_ActionClasses
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_ActionClasses);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Action_Setup;

	private:
		TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Classes;

	public:
		CK_PROPERTY_GET(_Classes);
	};

// ====================================================================================================================
// DEFINITION — This Action's own def, CDO-extracted at Setup time.
// In the unified model, every Action entity carries its OWN def (one def per
// Action). The parent's planner consumes its children's defs as candidate
// operators.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Definition
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Definition);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;
		friend class FProcessor_Goap_Action_HandleResult;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		// This Action's own def — extracted once from the CDO at Setup time.
		TArray<ck::goap::FWorldStateCondition_Raw> _Preconditions;
		TArray<ck::goap::FWorldStateEffect_Raw>    _Effects;
		float _Cost = 1.0f;

		// Pre-resolved form of _Effects as the Action's _Goal when active.
		// Populated at Setup; read at chain extension. (Will be filled by U3.)
		TArray<FCk_GoapWS_Condition_Authored> _GoalFromEffects;

		// Effect keys not registered in the resolved WS — populated at Setup,
		// surfaced via Get_InvalidGoal. (Will be filled by U5.)
		TArray<FCk_GoapWS_Condition_Authored> _InvalidGoal;

		// Pre-built FActionDef cache used by parent's planner as a candidate
		// operator. Built once at Setup time. (Will be filled by U3.)
		ck::goap::FActionDef _CachedActionDef;

	public:
		CK_PROPERTY_GET(_Preconditions);
		CK_PROPERTY_GET(_Effects);
		CK_PROPERTY_GET(_Cost);
		CK_PROPERTY_GET(_GoalFromEffects);
		CK_PROPERTY_GET(_InvalidGoal);

		auto AsActionDef() const -> const ck::goap::FActionDef& { return _CachedActionDef; }
	};

// ====================================================================================================================
// TREE — Parent/child relationships between Action entities. Populated at
// AddAction_ToAction (Phase U2 helper) and ChainUpdate time.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Tree
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Tree);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleResult;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		FCk_Handle_Goap_Action _ParentAction;   // invalid for top-level Actions
		TArray<FCk_Handle_Goap_Action> _ChildActions;

	public:
		CK_PROPERTY_GET(_ParentAction);
		CK_PROPERTY_GET(_ChildActions);
	};

// ====================================================================================================================
// REQUESTS — per-action request queue
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Requests
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Requests);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Action_HandleRequests;
		friend class FProcessor_Goap_Action_AutoReplan;

		using RequestType = std::variant<
			FCk_Request_Goap_Action_Plan,
			FCk_Request_Goap_Action_CancelPlan,
			FCk_Request_Goap_Action_SetGoal,
			FCk_Request_Goap_Planner_SetGoal,
			FCk_Request_Goap_Action_SetActionCost,
			FCk_Request_Goap_Action_SetReplanInterval,
			FCk_Request_Goap_Action_SetReplanPolicy,
			FCk_Request_Goap_Action_SetSearchBudget,
			FCk_Request_Goap_Action_SetCostThreshold>;

	private:
		TArray<RequestType> _Requests;

	public:
		CK_PROPERTY_GET(_Requests);
	};

// ====================================================================================================================
// REPLAN THROTTLE — same shape as today's, per-action
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_ReplanThrottle
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_ReplanThrottle);

		friend class FProcessor_Goap_Action_AutoReplan;
		friend class FProcessor_Goap_Action_HandleRequests;

	private:
		float _SecondsSinceLastReplan = 0.0f;

	public:
		CK_PROPERTY_GET(_SecondsSinceLastReplan);
	};

// ====================================================================================================================
// PLAN CONTEXT — Graph reference kept alive between search + result phases
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_PlanContext
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_PlanContext);

		friend class FProcessor_Goap_Action_HandleRequests;
		friend class FProcessor_Goap_Action_HandleResult;

	private:
		goap::FGoapGraph _Graph;

	public:
		CK_PROPERTY_GET(_Graph);
	};

// ====================================================================================================================
// A* FRAGMENT ALIASES — concrete types per action
// ====================================================================================================================

	using FFragment_Goap_Action_SearchState = TFragment_AStar_SearchState<int32, goap::FGoapGraph>;
	using FFragment_Goap_Action_Result      = TFragment_AStar_Result<int32>;

// ====================================================================================================================
// SIGNALS — Action-scoped
// ====================================================================================================================

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Action_PlanComplete,
		FCk_Delegate_Goap_OnActionPlanComplete,
		FCk_Handle_Goap_Action,
		FCk_Goap_Payload_OnPlanComplete);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Action_PlanFailed,
		FCk_Delegate_Goap_OnActionPlanFailed,
		FCk_Handle_Goap_Action,
		FCk_Goap_Payload_OnPlanFailed);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Action_Activated,
		FCk_Delegate_Goap_OnActionActivated,
		FCk_Handle_Goap_Action,
		FCk_Goap_Payload_OnActionActivated);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Action_Deactivated,
		FCk_Delegate_Goap_OnActionDeactivated,
		FCk_Handle_Goap_Action,
		FCk_Goap_Payload_OnActionDeactivated);

// ====================================================================================================================

} // namespace ck
