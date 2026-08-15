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
// Defers (keeping RequiresSetup) while any catalog Action still awaits its own Setup.

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
	// Per-Action Setup populates the WS key registry that Planner-tier goal resolution looks up.
	using RunAfter = TDepList<FProcessor_Goap_Action_Setup>;

public:
	using TProcessor::TProcessor;

public:
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

class CKGOAP_API FProcessor_Goap_Planner_UpdateActivation : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_UpdateActivation,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Current>,
	ck::TReadOnly<FFragment_Goap_Planner_PlanState>,
	ck::TReadWrite<FFragment_Goap_Planner_Activation>,
	FTag_Goap_Planner_ActivationDirty,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Planner_HandleResult>;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Activation& InActivation) const -> void;

public:
	static auto
	DoActivatePlanner(
		FCk_Handle_Goap_Action InPlanner,
		FCk_Handle_Goap_Action InParent) -> void;

	// Also deactivates active descendants recursively — otherwise their stale _IsActive would
	// make a later re-activation of the same sub-tree silently no-op the descendant chain.
	static auto
	DoDeactivatePlanner(FCk_Handle_Goap_Action InPlanner) -> void;

private:
	static auto
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InPlanner) -> void;

	static auto
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent) -> void;

	static auto
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void;

	static auto
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
