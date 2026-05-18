#pragma once

#include "CkGoap_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Processor.h"

#include "CkAStar/CkAStar_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — Extract action/goal metadata from CDOs into lightweight fragments
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_Setup,
	FCk_Handle_Goap,
	ck::TReadOnly<FFragment_Goap_Params>,
	ck::TReadOnly<FFragment_Goap_ActionClasses>,
	ck::TReadOnly<FFragment_Goap_GoalClasses>,
	ck::TReadWrite<FFragment_Goap_Actions>,
	ck::TReadWrite<FFragment_Goap_Goals>,
	ck::TReadWrite<FFragment_Goap_Diagnostics>,
	FTag_Goap_RequiresSetup,
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
		const FFragment_Goap_Params& InParams,
		const FFragment_Goap_ActionClasses& InActionClasses,
		const FFragment_Goap_GoalClasses& InGoalClasses,
		FFragment_Goap_Actions& InActions,
		FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Diagnostics& InDiagnostics) -> void;
};

// ====================================================================================================================
// HANDLE REQUESTS — Process Plan / SetWorldState / CancelPlan
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_HandleRequests : public ck_exp::TProcessor<
	FProcessor_Goap_HandleRequests,
	FCk_Handle_Goap,
	ck::TReadWrite<FFragment_Goap_Actions>,
	ck::TReadOnly<FFragment_Goap_Goals>,
	ck::TReadWrite<FFragment_Goap_Params>,
	ck::TReadWrite<FFragment_AStar_Params>,
	ck::TReadWrite<FFragment_Goap_Current>,
	ck::TReadOnly<FFragment_Goap_Requests>,
	ck::TReadWrite<FFragment_Goap_SearchState>,
	ck::TReadWrite<FFragment_Goap_Result>,
	ck::TReadWrite<FFragment_Goap_PlanContext>,
	ck::TReadWrite<FFragment_Goap_Diagnostics>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	// WorldState's request processor must run before the planner so the
	// source WS is current when IsGoalSatisfied / A* reads it.
	using RunAfter = TDepList<
		FProcessor_Goap_Setup,
		class FProcessor_Goap_AutoReplan,
		FProcessor_Goap_WorldState_HandleRequests>;
	using MarkedDirtyBy = FFragment_Goap_Requests;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Current& InCurrent,
		const FFragment_Goap_Requests& InRequests,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics) const -> void;

private:
	static auto
	DoHandleRequest(
		HandleType InHandle,
		const FFragment_Goap_Params& InParams,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics,
		const FCk_Request_Goap_Plan& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		const FCk_Request_Goap_CancelPlan& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Actions& InActions,
		const FCk_Request_Goap_SetActionCost& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		const FCk_Request_Goap_SetReplanInterval& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		const FCk_Request_Goap_SetReplanPolicy& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		const FCk_Request_Goap_SetSearchBudget& InRequest) -> void;

	static auto
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		const FCk_Request_Goap_SetCostThreshold& InRequest) -> void;
};

// ====================================================================================================================
// AUTO-REPLAN — Accumulate throttle time; enqueue Plan request per policy + dirty state
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_AutoReplan : public ck_exp::TProcessor<
	FProcessor_Goap_AutoReplan,
	FCk_Handle_Goap,
	ck::TReadOnly<FFragment_Goap_Params>,
	ck::TReadWrite<FFragment_Goap_ReplanThrottle>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Setup>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Params& InParams,
		FFragment_Goap_ReplanThrottle& InThrottle) -> void;
};

// ====================================================================================================================
// EXECUTE — Time-sliced A* search (parallel processor, pure data)
// ====================================================================================================================

struct FProcessor_Goap_Execute
	: TProcessor_AStar_Execute<FProcessor_Goap_Execute,
		  FCk_Handle_Goap, FFragment_Goap_SearchState, FFragment_Goap_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_HandleRequests>;
};

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_HandleResult : public ck_exp::TProcessor<
	FProcessor_Goap_HandleResult,
	FCk_Handle_Goap,
	ck::TReadOnly<FFragment_Goap_Result>,
	ck::TReadOnly<FFragment_Goap_PlanContext>,
	ck::TReadWrite<FFragment_Goap_Current>,
	FTag_AStar_SearchComplete,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Execute>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Result& InResult,
		const FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Current& InCurrent) -> void;
};

// ====================================================================================================================
// ENDPLAY — Cleanup search state on entity destruction
// ====================================================================================================================

struct FProcessor_Goap_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_Goap_EndPlay,
		  FCk_Handle_Goap, FFragment_Goap_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_EndPlay;
};

// ====================================================================================================================

} // namespace ck
