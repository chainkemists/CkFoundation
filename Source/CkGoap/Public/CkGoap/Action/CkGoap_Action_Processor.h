#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"

#include "CkAStar/CkAStar_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{
	// Forward decl — Planner-tier Setup processor lives in
	// CkGoap_Planner_Processor.h. We need to refer to it in RunAfter lists
	// here so the Planner-on-Planner processors order after it.
	class FProcessor_Goap_Planner_Setup;

// ====================================================================================================================
// SETUP — Extract action CDOs into FActionDef list; resolve raw tags via the
//         tier's resolved WS-source registry; seed root tier's initial goal.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Action_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_Action_Setup,
	FCk_Handle_Goap_Action,
	ck::TReadOnly<FFragment_Goap_Action_Params>,
	ck::TReadWrite<FFragment_Goap_Action_Definition>,
	ck::TReadWrite<FFragment_Goap_Planner_WorldStateSource>,
	FTag_Goap_Action_RequiresSetup,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;

public:
	using TProcessor::TProcessor;

public:
	// PR-B.1b Stage 3: per-Action CDO extraction stays Action-side. Each Action
	// entity carries its own _Definition (CDO-extracted preconditions/effects/
	// cost), used by the Planner-on-Planner HandleRequests as a candidate
	// operator. Planner-side goal resolution (_GoalAuthored → _Goal) moved to
	// FProcessor_Goap_Planner_Setup (matches Planner directly).
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Planner_WorldStateSource& InWSSource) -> void;
};

// ====================================================================================================================
// AUTO REPLAN — Throttle + policy + initial-plan dispatch
//
// PR-B.1b Stage 3: matches Planner. Reads PlannerParams for replan policy/
// interval; Planner-side ReplanThrottle. Disable gate reads Planner's own
// FFragment_Goap_Planner_Current directly (no helper walk).
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Planner_AutoReplan : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_AutoReplan,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Params>,
	ck::TReadOnly<FFragment_Goap_Planner_Current>,
	ck::TReadOnly<FFragment_Goap_Planner_WorldStateSource>,
	ck::TReadWrite<FFragment_Goap_Planner_ReplanThrottle>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Action_Setup, FProcessor_Goap_Planner_Setup>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_ReplanThrottle& InThrottle) -> void;
};

// ====================================================================================================================
// HANDLE REQUESTS — Drain Planner-side request queue
//
// PR-B.1b Stage 3: matches Planner. Drains the Planner-side request queue,
// reads Planner-side PlanState/Goal/WorldStateSource/SearchState/Result/
// PlanContext. Candidate operators come from the implicit-root Action's
// Tree.Get_ChildActions() (top-level Planner) or the Planner's own
// Tree.Get_ChildActions() (promoted mid-tier Planner). Each child Action
// still carries its own FFragment_Goap_Action_Definition (CDO-extracted by
// the per-Action Setup processor below).
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Planner_HandleRequests : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_HandleRequests,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Params>,
	ck::TReadOnly<FFragment_Goap_Planner_Current>,
	ck::TReadWrite<FFragment_AStar_Params>,
	ck::TReadWrite<FFragment_Goap_Planner_PlanState>,
	ck::TReadWrite<FFragment_Goap_Planner_Goal>,
	ck::TReadWrite<FFragment_Goap_Planner_WorldStateSource>,
	ck::TReadOnly<FFragment_Goap_Planner_Requests>,
	ck::TReadWrite<FFragment_Goap_Planner_SearchState>,
	ck::TReadWrite<FFragment_Goap_Planner_Result>,
	ck::TReadWrite<FFragment_Goap_Planner_PlanContext>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<
		FProcessor_Goap_Action_Setup,
		FProcessor_Goap_Planner_Setup,
		FProcessor_Goap_Planner_AutoReplan,
		FProcessor_Goap_WorldState_HandleRequests>;
	using MarkedDirtyBy = FFragment_Goap_Planner_Requests;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Goal& InGoal,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		const FFragment_Goap_Planner_Requests& InRequests,
		FFragment_Goap_Planner_SearchState& InSearchState,
		FFragment_Goap_Planner_Result& InResult,
		FFragment_Goap_Planner_PlanContext& InPlanContext) const -> void;
};

// ====================================================================================================================
// EXECUTE — Time-sliced A* search (parallel; pure data)
//
// PR-B.1b Stage 3: instantiated on Planner entity.
// ====================================================================================================================

struct FProcessor_Goap_Planner_Execute
	: TProcessor_AStar_Execute<FProcessor_Goap_Planner_Execute,
		FCk_Handle_Goap_Planner, FFragment_Goap_Planner_SearchState, FFragment_Goap_Planner_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Planner_HandleRequests>;
};

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
//
// PR-B.1b Stage 3: matches Planner. Writes to Planner's PlanState. Broadcasts
// per-Planner signals directly from the Planner handle (no resolver walk).
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Planner_HandleResult : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_HandleResult,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Params>,
	ck::TReadOnly<FFragment_Goap_Planner_Current>,
	ck::TReadOnly<FFragment_Goap_Planner_Result>,
	ck::TReadOnly<FFragment_Goap_Planner_PlanContext>,
	ck::TReadWrite<FFragment_Goap_Planner_PlanState>,
	FTag_AStar_SearchComplete,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Planner_Execute>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_Result& InResult,
		const FFragment_Goap_Planner_PlanContext& InPlanContext,
		FFragment_Goap_Planner_PlanState& InPlanState) -> void;
};

// ====================================================================================================================
// END PLAY — Clean up A* search state on entity destruction
// ====================================================================================================================

struct FProcessor_Goap_Planner_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_Goap_Planner_EndPlay,
		FCk_Handle_Goap_Planner, FFragment_Goap_Planner_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_EndPlay;
};

// ====================================================================================================================

} // namespace ck
