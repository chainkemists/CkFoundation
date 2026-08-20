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

// Befriended below on FFragment_Goap_Planner_WorldStateSource.
namespace ck::goap::internal_planner
{
	CKGOAP_API auto DoResolveChildWorldStateFromParent(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParentAction,
		EResolveWorldStateSourcePolicy InPolicy) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class FProcessor_Goap_Planner_Setup;
	class FProcessor_Goap_Planner_UpdateActivation;
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

	// Added by HandleRequests when A* is seeded; removed by HandleResult on any
	// terminal status or by Request_CancelPlan. Gates child Planners from draining
	// their own Plan requests while the parent's plan is in flight.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_PlanInFlight);

// --------------------------------------------------------------------------------------------------------------------

	using FFragment_Goap_Planner_Params = FCk_Fragment_Goap_PlannerParamsData;

// --------------------------------------------------------------------------------------------------------------------

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

		// True iff the catalog holds an Action with no preconditions whose effects
		// cover every goal condition. Gates the PlanFailed runtime ensure; computed
		// once by FProcessor_Goap_Planner_Setup.
		bool _HasUnconditionalFallback = false;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
		CK_PROPERTY_GET(_HasUnconditionalFallback);
	};

// --------------------------------------------------------------------------------------------------------------------

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
// O(1) tag-to-action lookup, populated at AddAction time.

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

		// Friendship is class-scoped and doesn't reach namespace-level free
		// functions, so ck::goap::internal_planner goes through this.
		auto AddEntry(FGameplayTag InTag, FCk_Handle_Goap_Action InAction) -> void
		{
			_TagToAction.Add(InTag, InAction);
		}

		// Returns the number of removed entries (0 if the tag was not registered).
		auto RemoveEntry(FGameplayTag InTag) -> int32
		{
			return _TagToAction.Remove(InTag);
		}
	};

// --------------------------------------------------------------------------------------------------------------------

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

		friend auto goap::internal_planner::DoResolveChildWorldStateFromParent(
			FCk_Handle_Goap_Action& InChild,
			const FCk_Handle_Goap_Action& InParentAction,
			goap::internal_planner::EResolveWorldStateSourcePolicy InPolicy) -> void;

	private:
		// Planner-level default; falls through to children whose own
		// _WorldStateSource_Override is unset. On Action entities this field is
		// unused (their override lives on Params) — only _Resolved is meaningful.
		FCk_Handle_Goap_WorldState _WorldStateSource;

		// Resolved at activation: override, else parent's resolved, else Planner WS.
		FCk_Handle_Goap_WorldState _Resolved;

	public:
		CK_PROPERTY_GET(_WorldStateSource);
		CK_PROPERTY_SET(_WorldStateSource);
		CK_PROPERTY_GET(_Resolved);
	};

// --------------------------------------------------------------------------------------------------------------------

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

		// Action *entities*; Get_PlanClasses() maps them back to EntityScript classes.
		TArray<FCk_Handle_Goap_Action>                   _Plan;

		float                                            _PlanCost = 0.0f;
		int32                                            _PlanAttemptCount = 0;

	public:
		CK_PROPERTY_GET(_PlanStatus);
		CK_PROPERTY_GET(_Plan);
		CK_PROPERTY_GET(_PlanCost);
		CK_PROPERTY_GET(_PlanAttemptCount);

		auto Get_PlanClasses() const -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>;

		/**
		 * The class of the first VALID action in the plan, or an empty class when the plan holds
		 * none.
		 *
		 * Equals Get_PlanClasses()[0] — including the identical skip over invalid handles —
		 * WHENEVER that array is non-empty; where it would be empty, indexing [0] is out of bounds
		 * and this returns an empty class instead. Avoids building the whole array, which callers
		 * that only want the head were paying for once or twice per agent per frame.
		 */
		auto Get_FirstPlanClass() const -> TSubclassOf<UCk_GoapAction_EntityScript>;
	};

// --------------------------------------------------------------------------------------------------------------------
// _GoalAuthored (tag-keyed) is the source of truth; _Goal is its registry-keyed
// resolution used by A*. Setup resolves it; ChainUpdate re-resolves on activation
// when the WS source may differ. An empty _GoalAuthored means PlanFound + empty plan.

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
		// Persists across chain (de)activations: re-resolution reads this field,
		// never any Action's effects.
		TArray<FCk_GoapWS_Condition_Authored>            _GoalAuthored;

		TArray<goap::FWorldStateCondition>               _Goal;
		TArray<FCk_GoapWS_Condition_Authored>            _InvalidGoal;

	public:
		CK_PROPERTY_GET(_GoalAuthored);
		CK_PROPERTY_GET(_Goal);
		CK_PROPERTY_GET(_InvalidGoal);
	};

// --------------------------------------------------------------------------------------------------------------------
// Source type is FCk_Handle_Goap_Planner, but signal storage lives on the
// underlying entity that runs A* — Bind/Unbind resolve Planner → that entity.

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
// Stamped by HandleRequests; read by the debugger via Get_LastReplanCause.

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
// World-time stamp of the Planner's last replan. A stamp rather than a per-frame accumulator so the
// throttle survives AutoReplan not visiting this planner every frame. Stamped with the CURRENT world
// time where the fragment is added, which reproduces the old accumulator exactly (it started at 0 on
// add); the very first plan bypasses the throttle anyway via FTag_Goap_Planner_RequiresInitialPlan.

	struct CKGOAP_API FFragment_Goap_Planner_ReplanThrottle
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ReplanThrottle);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_AutoReplan;
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		double _LastReplanWorldTimeSeconds = 0.0;

	public:
		CK_PROPERTY_GET(_LastReplanWorldTimeSeconds);
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

	using FFragment_Goap_Planner_SearchState = TFragment_AStar_SearchState<int32, goap::FGoapGraph>;
	using FFragment_Goap_Planner_Result      = TFragment_AStar_Result<int32>;

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
