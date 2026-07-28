#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"

#include "CkAStar/CkAStar_Processor.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	// Defined in CkGoap_Planner_Processor.h; needed by the RunAfter lists below.
	class FProcessor_Goap_Planner_Setup;

// --------------------------------------------------------------------------------------------------------------------
// Extract each Action's CDO into its own FActionDef

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
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Planner_WorldStateSource& InWSSource) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Throttle + policy + initial-plan dispatch

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

// --------------------------------------------------------------------------------------------------------------------
// Drain Planner-side request queue

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
	TExclude<FTag_DestroyEntity_Initiate>,
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

// --------------------------------------------------------------------------------------------------------------------
// HandleRequests excludes owners already tagged for destruction, so a destroyed Planner's still-queued
// requests are never drained. This fires each pending request's completion delegate with
// Failed_Cancelled so a caller awaiting completion terminates instead of hanging.

class CKGOAP_API FProcessor_Goap_Planner_CancelPendingRequests : public ck_exp::TProcessor<
	FProcessor_Goap_Planner_CancelPendingRequests,
	FCk_Handle_Goap_Planner,
	ck::TReadOnly<FFragment_Goap_Planner_Requests>,
	CK_IF_END_PLAY>
{
public:
	using Group = FGroup_EndPlay;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Requests& InRequestsComp)
		-> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Time-sliced A* search (parallel; pure data)

struct FProcessor_Goap_Planner_Execute
	: TProcessor_AStar_Execute<FProcessor_Goap_Planner_Execute,
		FCk_Handle_Goap_Planner, FFragment_Goap_Planner_SearchState, FFragment_Goap_Planner_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Planner_HandleRequests>;
};

// --------------------------------------------------------------------------------------------------------------------
// Convert A* path to action sequence, fire signals

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

// --------------------------------------------------------------------------------------------------------------------
// Clean up A* search state on entity destruction

struct FProcessor_Goap_Planner_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_Goap_Planner_EndPlay,
		FCk_Handle_Goap_Planner, FFragment_Goap_Planner_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_EndPlay;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
