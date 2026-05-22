#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapDiagnostic_DependencyCycle
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"  // FCk_Handle_Goap_Action, FCk_GoapWS_Condition_Authored, ECk_GoapPlanStatus
#include "CkGoap/Action/CkGoap_Action_Fragment.h"        // PR-B.1b prep: Planner-side aliases reuse Action-side fragment types
#include "CkGoap/Algorithm/CkGoap_WorldState.h"  // goap::FWorldStateCondition
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Planner_UE;
class UCk_Utils_Goap_Action_UE;
class UCk_GoapAction_EntityScript;

// Forward decl for the PR-A internal_planner helper befriended on
// FFragment_Goap_Planner_WorldStateSource below. Defined in
// CkGoap_Planner_Internal.h / CkGoap_Planner_Utils.cpp.
namespace ck::goap::internal_planner
{
	CKGOAP_API auto DoResolveChildWorldStateFromParent(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParentAction) -> void;
}

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_Planner_Setup;
	class FProcessor_Goap_Planner_UpdateActivation;
	class FProcessor_Goap_Action_Setup;
	class FProcessor_Goap_Action_HandleRequests;
	class FProcessor_Goap_Action_HandleResult;

// ====================================================================================================================
// TAGS
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresSetup);

	// Set whenever any action in the ActionSet completes a plan. Consumed +
	// removed by UpdateActivation. Optimization to skip walking inert ActionSets.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresChainUpdate);

// ====================================================================================================================
// PARAMS — alias to the BlueprintType data shape
// ====================================================================================================================

	using FFragment_Goap_Planner_Params = FCk_Fragment_Goap_PlannerParamsData;

// ====================================================================================================================
// CURRENT FRAGMENT — Runtime ActionSet state (enable toggle, diagnostics)
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Current);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_Setup;
		friend class FProcessor_Goap_Planner_UpdateActivation;

	private:
		ECk_EnableDisable _EnableToggle = ECk_EnableDisable::Enable;
		TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

		// The implicit-root Action entity for this top-level Planner — the
		// entity that actually runs A* in the transitional Path A model. Set by
		// the first AddAction call on a top-level Planner; subsequent AddActions
		// become tree children of this root. Unused for promoted mid-tier
		// Planners (the host entity itself runs A* and reads its own Tree).
		// PR-B is expected to rehost A* onto the Planner entity directly, at
		// which point this field can be removed.
		FCk_Handle_Goap_Action _RootAction;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
		CK_PROPERTY_GET(_RootAction);
	};

// ====================================================================================================================
// ACTIVATION — Per-Planner activation state. Used by UpdateActivation to
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
// ====================================================================================================================

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

// ====================================================================================================================
// ACTION CATALOG INDEX — O(1) tag-to-action lookup. Populated at AddAction
// time; read by lookup helpers (Find_Action, etc.).
// ====================================================================================================================

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

// ====================================================================================================================
// WORLD STATE SOURCE — ActionSet-level default WS source. Used by the unified
// ChainUpdate logic when an Action does not provide its own override.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_WorldStateSource
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_WorldStateSource);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;

		// PR-A: shared internal helper for AddAction's child-WS resolution.
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

// ====================================================================================================================
// PLAN STATE — Planner-role fragment: live plan + status + cost + attempt count
// for the planner running on this entity. Lives on every Action entity (because
// every Action runs its own planner in the unified model) and on the top-level
// Planner entity.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_PlanState
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_PlanState);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;
		friend class FProcessor_Goap_Action_HandleResult;

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

// ====================================================================================================================
// GOAL — Planner-role fragment: effective goal world state for this planner.
//
// U11.1: every Planner has its own _Goal, set at construction via
// FCk_Fragment_Goap_PlannerParamsData._Goal and mutable via Request_SetGoal.
// _GoalAuthored is the source-of-truth (authored, tag-keyed). _Goal is the
// resolved form (registry-keyed) used by the A* planner. Setup resolves
// _GoalAuthored → _Goal; ChainUpdate re-resolves on activation when the WS
// source may differ. There is no longer any implicit "goal = effects" rule —
// a Planner with an empty _GoalAuthored has an empty _Goal (planner emits an
// empty plan / PlanFound immediately).
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_Goal
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Goal);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;

	private:
		// Authored (tag-keyed) goal — source of truth, settable at construction
		// (PlannerParams._Goal — propagated to the implicit-root Action on the
		// first AddAction call for a top-level Planner) and at runtime
		// (Request_SetGoal). Persists across chain (de)activations of the
		// owning Action — DoInjectGoalSynchronous re-resolves from this field,
		// not from any Action's effects.
		TArray<FCk_GoapWS_Condition_Authored>            _GoalAuthored;

		TArray<goap::FWorldStateCondition>               _Goal;
		TArray<FCk_GoapWS_Condition_Authored>            _InvalidGoal;

	public:
		CK_PROPERTY_GET(_GoalAuthored);
		CK_PROPERTY_GET(_Goal);
		CK_PROPERTY_GET(_InvalidGoal);
	};

// ====================================================================================================================
// SIGNALS — ActionSet-scoped
// ====================================================================================================================

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_ActiveChainChanged,
		FCk_Delegate_Goap_OnActiveChainChanged,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnActiveChainChanged);

// ====================================================================================================================
// PR-B.1b prep — Planner-side fragment / tag aliases.
//
// These aliases name the A* pipeline fragments + lifecycle tags by their
// Planner-scoped names. In the transitional B.1b model the underlying types
// are still the Action-side ones (stamped on the implicit-root Action entity
// today); the aliases give us the future names to reference now. Subsequent
// PR-B.1b sub-commits will:
//   1. Stamp these fragments on the Planner entity (in addition / instead of
//      the Action entity) and retarget the A*-pipeline processors to read
//      them from the Planner entity.
//   2. Retire the Action-side stamps + `_RootAction` indirection.
//
// Adding the aliases now is a no-op behaviourally — the names refer to the
// same underlying types — but lets call-sites in subsequent commits be written
// against the destination names without churning the type definitions in the
// same patch.
// ====================================================================================================================

	using FFragment_Goap_Planner_Requests       = FFragment_Goap_Action_Requests;
	using FFragment_Goap_Planner_ReplanThrottle = FFragment_Goap_Action_ReplanThrottle;
	using FFragment_Goap_Planner_PlanContext    = FFragment_Goap_Action_PlanContext;
	using FFragment_Goap_Planner_SearchState    = FFragment_Goap_Action_SearchState;
	using FFragment_Goap_Planner_Result         = FFragment_Goap_Action_Result;

	// Lifecycle tags — Planner-scoped names for the A*-pipeline gating tags.
	// In the transitional model these are stamped on the implicit-root Action;
	// B.1b commits 2-3 move the stamps to the Planner entity.
	using FTag_Goap_Planner_PlanRequested       = FTag_Goap_Action_PlanRequested;
	using FTag_Goap_Planner_RequiresInitialPlan = FTag_Goap_Action_RequiresInitialPlan;
	using FTag_Goap_Planner_PlanInFlight        = FTag_Goap_Action_PlanInFlight;

// ====================================================================================================================

} // namespace ck
