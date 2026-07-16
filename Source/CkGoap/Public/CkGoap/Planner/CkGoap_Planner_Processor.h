#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------
// ActionSet-scoped: detect Action dependency cycles in the catalog.
// Runs once whenever the ActionSet's RequiresSetup tag is set. Defers if any
// catalog Action hasn't completed its own Setup yet.

class CKGOAP_API FProcessor_Goap_Planner_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_Setup,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Params>,
	ck::TReadWrite<FFragment_Goap_Planner_Current>,
	ck::TReadOnly<FFragment_Goap_Planner_ActionCatalogIndex>,
	ck::TReadWrite<FFragment_Goap_Planner_WorldStateSource>,
	ck::TReadWrite<FFragment_Goap_Planner_Goal>,
	FTag_Goap_Planner_RequiresSetup,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	// must run after per-Action Setup so the WS registry has
	// keys registered (Planner-tier goal resolution looks them up). Without
	// this, top-level Planner goal _Goal stays empty when keys aren't yet in
	// the registry, leading to empty plans / no OnPlanComplete fires.
	using RunAfter = TDepList<FProcessor_Goap_Action_Setup>;

public:
	using TProcessor::TProcessor;

public:
	// also resolves the Planner-side _GoalAuthored → _Goal
	// using the Planner's own resolved WS source. Resolution lives here (not
	// the per-Action Setup) because the goal is conceptually a Planner-level
	// concern. Defers if the Planner has no resolved WS source yet or if any
	// candidate child still has FTag_Goap_Action_RequiresSetup.
	//
	// Always-valid-plan tenet: also runs the static fallback check, caches the
	// result on InCurrent._HasUnconditionalFallback, and fires CK_ENSURE_IF_NOT
	// if no fallback exists and InParams._AllowPlanFailed == false. See
	// the module design-tenets notes.
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_ActionCatalogIndex& InCatalogIndex,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_Goal& InGoal) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Per-Planner activation transition. Each Planner (Action
// entity in the transitional model) caches its Plan[0] frame-over-frame.
// When Plan[0] changes, deactivate the old sub-Planner (if it was a composite
// child) and activate the new one. The "active chain" is no longer stored —
// it is derived on demand by walking Plan[0]s from the top via
// UCk_Utils_Goap_Planner_UE::Get_ActiveChain.
//
// Runs LAST in the AI group so HandleResult has populated this frame's plan
// before activation decisions are made.

class CKGOAP_API FProcessor_Goap_Planner_UpdateActivation : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_UpdateActivation,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Current>,
	ck::TReadOnly<FFragment_Goap_Planner_PlanState>,
	ck::TReadWrite<FFragment_Goap_Planner_Activation>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Planner_HandleResult>;

public:
	using TProcessor::TProcessor;

public:
	// matches Planner directly. Reads Planner-side PlanState.
	// Activation/deactivation of sub-Planners (children entities) still uses
	// FCk_Handle_Goap_Action handles — sub-Planners ARE Actions in Path A.
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Activation& InActivation) const -> void;

public:
	// Resolve a child sub-Planner's WS source (override → parent's resolved →
	// ActionSet WS), inject its resolved goal, subscribe to its WS, add
	// RequiresInitialPlan, broadcast OnPlannerActivated, and flip _IsActive.
	static auto
	DoActivatePlanner(
		FCk_Handle_Goap_Action InPlanner,
		FCk_Handle_Goap_Action InParent) -> void;

	// Tear down the sub-Planner's planning state (clears Plan, PlanStatus →
	// Idle, _ActiveParent, _Resolved WS, unsubscribes from WS), removes
	// RequiresInitialPlan + PlanInFlight, broadcasts OnPlannerDeactivated,
	// and flips _IsActive to false. Recursively deactivates active descendants
	// (whose own _IsActive was set by previous activation walks).
	static auto
	DoDeactivatePlanner(FCk_Handle_Goap_Action InPlanner) -> void;

private:
	// Re-resolve InPlanner's goal from its _GoalAuthored using its resolved
	// WS registry. Same semantics as the old DoInjectGoalSynchronous.
	static auto
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InPlanner) -> void;

	// Walks the WS-source override chain (override → parent's resolved →
	// top-level Planner's WS). Writes into InChild._Resolved.
	static auto
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent) -> void;

	// Add/remove an Action's handle to/from its resolved WS's subscriber list.
	static auto
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void;

	static auto
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
