#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"

#include "CkAStar/CkAStar_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — Extract action CDOs into FActionDef list; resolve raw tags via the
//         tier's resolved WS-source registry; seed root tier's initial goal.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Action_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_Action_Setup,
	FCk_Handle_Goap_Action,
	ck::TReadOnly<FFragment_Goap_Action_Params>,
	ck::TReadOnly<FFragment_Goap_Action_ActionClasses>,
	ck::TReadWrite<FFragment_Goap_Action_Definition>,
	ck::TReadWrite<FFragment_Goap_Action_Current>,
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
		const FFragment_Goap_Action_ActionClasses& InClasses,
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Action_Current& InCurrent) -> void;
};

// ====================================================================================================================
// AUTO REPLAN — Throttle + policy + initial-plan dispatch
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Action_AutoReplan : public ck_exp::TProcessor<
	FProcessor_Goap_Action_AutoReplan,
	FCk_Handle_Goap_Action,
	ck::TReadOnly<FFragment_Goap_Action_Params>,
	ck::TReadWrite<FFragment_Goap_Action_ReplanThrottle>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Action_Setup>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_Goap_Action_ReplanThrottle& InThrottle) -> void;
};

// ====================================================================================================================
// HANDLE REQUESTS — Drain per-tier request queue
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Action_HandleRequests : public ck_exp::TProcessor<
	FProcessor_Goap_Action_HandleRequests,
	FCk_Handle_Goap_Action,
	ck::TReadOnly<FFragment_Goap_Action_Params>,
	ck::TReadWrite<FFragment_AStar_Params>,
	ck::TReadWrite<FFragment_Goap_Action_Current>,
	ck::TReadWrite<FFragment_Goap_Action_Definition>,
	ck::TReadOnly<FFragment_Goap_Action_Requests>,
	ck::TReadWrite<FFragment_Goap_Action_SearchState>,
	ck::TReadWrite<FFragment_Goap_Action_Result>,
	ck::TReadWrite<FFragment_Goap_Action_PlanContext>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<
		FProcessor_Goap_Action_Setup,
		FProcessor_Goap_Action_AutoReplan,
		FProcessor_Goap_WorldState_HandleRequests>;
	using MarkedDirtyBy = FFragment_Goap_Action_Requests;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Action_Current& InCurrent,
		FFragment_Goap_Action_Definition& InActionDef,
		const FFragment_Goap_Action_Requests& InRequests,
		FFragment_Goap_Action_SearchState& InSearchState,
		FFragment_Goap_Action_Result& InResult,
		FFragment_Goap_Action_PlanContext& InPlanContext) const -> void;
};

// ====================================================================================================================
// EXECUTE — Time-sliced A* search (parallel; pure data)
// ====================================================================================================================

struct FProcessor_Goap_Action_Execute
	: TProcessor_AStar_Execute<FProcessor_Goap_Action_Execute,
		FCk_Handle_Goap_Action, FFragment_Goap_Action_SearchState, FFragment_Goap_Action_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Action_HandleRequests>;
};

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Action_HandleResult : public ck_exp::TProcessor<
	FProcessor_Goap_Action_HandleResult,
	FCk_Handle_Goap_Action,
	ck::TReadOnly<FFragment_Goap_Action_Result>,
	ck::TReadOnly<FFragment_Goap_Action_PlanContext>,
	ck::TReadWrite<FFragment_Goap_Action_Current>,
	FTag_AStar_SearchComplete,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Action_Execute>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Result& InResult,
		const FFragment_Goap_Action_PlanContext& InPlanContext,
		FFragment_Goap_Action_Current& InCurrent) -> void;
};

// ====================================================================================================================
// END PLAY — Clean up A* search state on entity destruction
// ====================================================================================================================

struct FProcessor_Goap_Action_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_Goap_Action_EndPlay,
		FCk_Handle_Goap_Action, FFragment_Goap_Action_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_EndPlay;
};

// ====================================================================================================================

} // namespace ck
