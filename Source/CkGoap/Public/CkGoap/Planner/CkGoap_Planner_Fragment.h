#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapDiagnostic_DependencyCycle
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"  // FCk_Handle_Goap_Action, FCk_GoapWS_Condition_Authored, ECk_GoapPlanStatus
#include "CkGoap/Action/CkGoap_Action_Fragment.h"        // CDO defs the Planner consumes from child Actions
#include "CkGoap/Algorithm/CkGoap_WorldState.h"  // goap::FWorldStateCondition
#include "CkGoap/Algorithm/CkGoap_Graph.h"        // goap::FGoapGraph (FFragment_Goap_Planner_PlanContext)
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkAStar/CkAStar_Fragment.h"             // TFragment_AStar_SearchState / _Result aliases
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Planner_UE;
class UCk_Utils_Goap_Action_UE;
class UCk_GoapAction_EntityScript;

// Forward decl for the internal_planner helper befriended on
// FFragment_Goap_Planner_WorldStateSource below. Defined in
// CkGoap_Planner_Internal.h / CkGoap_Planner_Utils.cpp.
namespace ck::goap::internal_planner
{
	CKGOAP_API auto DoResolveChildWorldStateFromParent(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParentAction) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class FProcessor_Goap_Planner_Setup;
	class FProcessor_Goap_Planner_UpdateActivation;
	// Planner-side A*-pipeline processors.
	class FProcessor_Goap_Planner_AutoReplan;
	class FProcessor_Goap_Planner_HandleRequests;
	class FProcessor_Goap_Planner_HandleResult;
	class FProcessor_Goap_Action_Setup;

// --------------------------------------------------------------------------------------------------------------------

	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresSetup);

	// Set whenever any action in the ActionSet completes a plan. Consumed +
	// removed by UpdateActivation. Optimization to skip walking inert ActionSets.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresChainUpdate);

	// Set on a Planner after activation; AutoReplan picks it up next frame to
	// fire the first plan request. Removed by AutoReplan once consumed.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresInitialPlan);

	// Request-flow gate — set when a Plan request lands on the queue.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_PlanRequested);

	// Set on a Planner while it is actively planning — added by HandleRequests
	// when a Plan request begins processing (AStar seeded), removed by
	// HandleResult on terminal status (PlanFound / PlanFailed /
	// CostThresholdReached) or by Request_CancelPlan. Used to gate child
	// Planners from draining their own Plan requests while the parent's plan
	// is in flight.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_PlanInFlight);

// --------------------------------------------------------------------------------------------------------------------
// Alias to the BlueprintType data shape

	using FFragment_Goap_Planner_Params = FCk_Fragment_Goap_PlannerParamsData;

// --------------------------------------------------------------------------------------------------------------------
// Runtime ActionSet state (enable toggle, diagnostics)

	struct CKGOAP_API FFragment_Goap_Planner_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Current);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_Setup;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Planner_HandleResult;
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		ECk_EnableDisable _EnableToggle = ECk_EnableDisable::Enable;
		TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

		// "Always-valid-plan" tenet — Setup-time cached result. True iff the
		// Planner's catalog contains at least one Action with no preconditions
		// whose effects cover every goal condition. Read by HandleResult /
		// HandleRequests at the PlanFailed branches to gate the runtime ensure
		// (when combined with PlannerParams._AllowPlanFailed=false). See
		// FProcessor_Goap_Planner_Setup for the static check and the module's
		// design tenets for the rationale.
		bool _HasUnconditionalFallback = false;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
		CK_PROPERTY_GET(_HasUnconditionalFallback);
	};

// --------------------------------------------------------------------------------------------------------------------
// Per-Planner activation state. Used by UpdateActivation to
// detect Plan[0] changes frame-over-frame and drive sub-Planner
// activate/deactivate transitions.
//
// _LastActivatedPlan0 — the Plan[0] handle this Planner saw on its previous
// tick. Compared against the current Plan[0] to detect changes.
//
// _IsActive — whether this Planner has been activated by a parent (or, for
// top-level Planners, by virtue of being top-level). Inactive Planners do not
// participate in the activation walk; they are mid-tier Planners awaiting
// their parent to select them as Plan[0].

	struct CKGOAP_API FFragment_Goap_Planner_Activation
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Activation);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;

	private:
		FCk_Handle_Goap_Action _LastActivatedPlan0;
		bool _IsActive = false;

	public:
		CK_PROPERTY_GET(_LastActivatedPlan0);
		CK_PROPERTY_GET(_IsActive);
	};

// --------------------------------------------------------------------------------------------------------------------
// O(1) tag-to-action lookup. Populated at AddAction
// time; read by lookup helpers (Find_Action, etc.).

	struct CKGOAP_API FFragment_Goap_Planner_ActionCatalogIndex
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ActionCatalogIndex);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;

	private:
		TMap<FGameplayTag, FCk_Handle_Goap_Action> _TagToAction;

	public:
		CK_PROPERTY_GET(_TagToAction);

		// Public mutator used by the shared entity-creation helper
		// ck::goap::internal_planner::DoCreateOrFindActionEntity. Private-
		// field-access via friendship is class-scoped and doesn't reach
		// namespace-level free functions, so the helper goes through this.
		auto AddEntry(FGameplayTag InTag, FCk_Handle_Goap_Action InAction) -> void
		{
			_TagToAction.Add(InTag, InAction);
		}

		// Public mutator counterpart for runtime catalog removal — used by
		// UCk_Utils_Goap_Planner_UE::Request_RemoveAction. Returns the number
		// of removed entries (0 if the tag was not registered).
		auto RemoveEntry(FGameplayTag InTag) -> int32
		{
			return _TagToAction.Remove(InTag);
		}
	};

// --------------------------------------------------------------------------------------------------------------------
// ActionSet-level default WS source. Used by the unified
// ChainUpdate logic when an Action does not provide its own override.

	struct CKGOAP_API FFragment_Goap_Planner_WorldStateSource
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_WorldStateSource);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Planner_Setup;
		friend class FProcessor_Goap_Planner_HandleRequests;
		friend class FProcessor_Goap_Action_Setup;

		// shared internal helper for AddAction's child-WS resolution.
		friend auto goap::internal_planner::DoResolveChildWorldStateFromParent(
			FCk_Handle_Goap_Action& InChild,
			const FCk_Handle_Goap_Action& InParentAction) -> void;

	private:
		// Planner-level default WS source. On top-level Planners, set by Add
		// from FCk_Fragment_Goap_PlannerParamsData._WorldStateSource. Falls
		// through to children when their own _WorldStateSource_Override is
		// unset. For the unified split model, Action entities carry this
		// fragment alongside their PlanState / Goal; on Actions _Resolved is
		// the per-Action eager-resolved source and _WorldStateSource is unused
		// (the override lives on Params).
		FCk_Handle_Goap_WorldState _WorldStateSource;

		// Resolved at activation: override-if-set, else parent's resolved, else
		// ActionSet WS. Lives here so the Action-role fragments need not duplicate.
		FCk_Handle_Goap_WorldState _Resolved;

	public:
		CK_PROPERTY_GET(_WorldStateSource);
		CK_PROPERTY_SET(_WorldStateSource);
		CK_PROPERTY_GET(_Resolved);
	};

// --------------------------------------------------------------------------------------------------------------------
// Planner-role fragment: live plan + status + cost + attempt count
// for the planner running on this entity. Lives on every Action entity (because
// every Action runs its own planner in the unified model) and on the top-level
// Planner entity.

	struct CKGOAP_API FFragment_Goap_Planner_PlanState
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_PlanState);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Planner_HandleRequests;
		friend class FProcessor_Goap_Planner_HandleResult;
		friend class FProcessor_Goap_Action_Setup;

	private:
		ECk_GoapPlanStatus                               _PlanStatus = ECk_GoapPlanStatus::Idle;

		// The planner emits a sequence of Action *entities*. Get_PlanClasses() is
		// a convenience mapping each entity back to its EntityScript class.
		TArray<FCk_Handle_Goap_Action>                   _Plan;

		float                                            _PlanCost = 0.0f;
		int32                                            _PlanAttemptCount = 0;

	public:
		CK_PROPERTY_GET(_PlanStatus);
		CK_PROPERTY_GET(_Plan);
		CK_PROPERTY_GET(_PlanCost);
		CK_PROPERTY_GET(_PlanAttemptCount);

		// Map each Action entity in the plan back to its EntityScript class.
		// Defined here (inline) to avoid creating a CkGoap_Planner_Fragment.cpp
		// just for one helper. Depends on FFragment_Goap_Action_Params so the
		// declaration lives in the .h that already includes that header.
		auto Get_PlanClasses() const -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>;
	};

// --------------------------------------------------------------------------------------------------------------------
// Planner-role fragment: effective goal world state for this Planner.
//
// Every Planner has its own _Goal, set at construction via
// FCk_Fragment_Goap_PlannerParamsData._Goal and mutable via Request_SetGoal.
// _GoalAuthored is the source-of-truth (authored, tag-keyed). _Goal is the
// resolved form (registry-keyed) used by the A* planner. Setup resolves
// _GoalAuthored → _Goal; ChainUpdate re-resolves on activation when the WS
// source may differ. There is no longer any implicit "goal = effects" rule —
// a Planner with an empty _GoalAuthored has an empty _Goal (planner emits an
// empty plan / PlanFound immediately).

	struct CKGOAP_API FFragment_Goap_Planner_Goal
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Goal);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Planner_Setup;
		friend class FProcessor_Goap_Planner_HandleRequests;
		friend class FProcessor_Goap_Action_Setup;

	private:
		// Authored (tag-keyed) goal — source of truth, settable at construction
		// (PlannerParams._Goal) and at runtime (Request_SetGoal). Persists
		// across chain (de)activations of the owning Planner —
		// DoInjectGoalSynchronous re-resolves from this field, never from any
		// Action's effects.
		TArray<FCk_GoapWS_Condition_Authored>            _GoalAuthored;

		TArray<goap::FWorldStateCondition>               _Goal;
		TArray<FCk_GoapWS_Condition_Authored>            _InvalidGoal;

	public:
		CK_PROPERTY_GET(_GoalAuthored);
		CK_PROPERTY_GET(_Goal);
		CK_PROPERTY_GET(_InvalidGoal);
	};

// --------------------------------------------------------------------------------------------------------------------
// Planner-scoped signals.
//
// Per-Planner signals have source type FCk_Handle_Goap_Planner. Under Path A
// the broadcast still happens on the underlying Action entity that runs A*
// (the implicit-root Action for top-level Planners, or the promoted host for
// mid-tier Planners) — the Bind/Unbind utilities resolve Planner → underlying
// entity so storage stays on the broadcasting entity. The payload's source
// handle is the Planner.

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_ActiveChainChanged,
		FCk_Delegate_Goap_OnActiveChainChanged,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnActiveChainChanged);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_PlanComplete,
		FCk_Delegate_Goap_OnPlanComplete,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnPlanComplete);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_PlanFailed,
		FCk_Delegate_Goap_OnPlanFailed,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnPlanFailed);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_Activated,
		FCk_Delegate_Goap_OnPlannerActivated,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnPlannerActivated);

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_Deactivated,
		FCk_Delegate_Goap_OnPlannerDeactivated,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnPlannerDeactivated);

// --------------------------------------------------------------------------------------------------------------------
// Planner-side request queue (Plan / CancelPlan / SetGoal / etc.)

	struct CKGOAP_API FFragment_Goap_Planner_Requests
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Requests);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_HandleRequests;
		friend class FProcessor_Goap_Planner_AutoReplan;

		using RequestType = std::variant<
			FCk_Request_Goap_Planner_Plan,
			FCk_Request_Goap_Planner_CancelPlan,
			FCk_Request_Goap_Planner_SetGoal,
			FCk_Request_Goap_Planner_SetActionCost,
			FCk_Request_Goap_Planner_RegisterActionCostProvider,
			FCk_Request_Goap_Planner_SetReplanInterval,
			FCk_Request_Goap_Planner_SetReplanPolicy,
			FCk_Request_Goap_Planner_SetSearchBudget,
			FCk_Request_Goap_Planner_SetCostThreshold>;

	private:
		TArray<RequestType> _Requests;

	public:
		CK_PROPERTY_GET(_Requests);
	};

// --------------------------------------------------------------------------------------------------------------------
// Why the last replan fired — stamped by HandleRequests when it consumes a
// Plan request; read by the debugger via Get_LastReplanCause.

	struct CKGOAP_API FFragment_Goap_Planner_ReplanCause
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ReplanCause);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		FCk_Goap_ReplanCauseInfo _Info;
		int64 _LastReplanFrame = 0;

	public:
		CK_PROPERTY_GET(_Info);
		CK_PROPERTY_GET(_LastReplanFrame);
	};

// --------------------------------------------------------------------------------------------------------------------
// Accumulator for the Planner's replan-interval window.

	struct CKGOAP_API FFragment_Goap_Planner_ReplanThrottle
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ReplanThrottle);

		friend class FProcessor_Goap_Planner_AutoReplan;
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		float _SecondsSinceLastReplan = 0.0f;

	public:
		CK_PROPERTY_GET(_SecondsSinceLastReplan);
	};

// --------------------------------------------------------------------------------------------------------------------
// Graph reference kept alive between search + result phases.

	struct CKGOAP_API FFragment_Goap_Planner_PlanContext
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_PlanContext);

		friend class FProcessor_Goap_Planner_HandleRequests;
		friend class FProcessor_Goap_Planner_HandleResult;

	private:
		goap::FGoapGraph _Graph;

	public:
		CK_PROPERTY_GET(_Graph);
	};

// --------------------------------------------------------------------------------------------------------------------
// A* FRAGMENT ALIASES — concrete CkAStar SearchState / Result types parameterised
// over goap::FGoapGraph. Each Planner entity carries one instance of each.

	using FFragment_Goap_Planner_SearchState = TFragment_AStar_SearchState<int32, goap::FGoapGraph>;
	using FFragment_Goap_Planner_Result      = TFragment_AStar_Result<int32>;

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
