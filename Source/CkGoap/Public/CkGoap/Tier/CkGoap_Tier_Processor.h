#pragma once

#include "CkGoap/Tier/CkGoap_Tier_Fragment.h"
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

class CKGOAP_API FProcessor_Goap_Tier_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_Tier_Setup,
	FCk_Handle_Goap_Tier,
	ck::TReadOnly<FFragment_Goap_Tier_Params>,
	ck::TReadOnly<FFragment_Goap_Tier_ActionClasses>,
	ck::TReadWrite<FFragment_Goap_Tier_Actions>,
	ck::TReadWrite<FFragment_Goap_Tier_Current>,
	FTag_Goap_Tier_RequiresSetup,
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
		const FFragment_Goap_Tier_Params& InParams,
		const FFragment_Goap_Tier_ActionClasses& InClasses,
		FFragment_Goap_Tier_Actions& InActions,
		FFragment_Goap_Tier_Current& InCurrent) -> void;
};

// ====================================================================================================================
// AUTO REPLAN — Throttle + policy + initial-plan dispatch
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Tier_AutoReplan : public ck_exp::TProcessor<
	FProcessor_Goap_Tier_AutoReplan,
	FCk_Handle_Goap_Tier,
	ck::TReadOnly<FFragment_Goap_Tier_Params>,
	ck::TReadWrite<FFragment_Goap_Tier_ReplanThrottle>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Tier_Setup>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Params& InParams,
		FFragment_Goap_Tier_ReplanThrottle& InThrottle) -> void;
};

// ====================================================================================================================
// HANDLE REQUESTS — Drain per-tier request queue
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Tier_HandleRequests : public ck_exp::TProcessor<
	FProcessor_Goap_Tier_HandleRequests,
	FCk_Handle_Goap_Tier,
	ck::TReadOnly<FFragment_Goap_Tier_Params>,
	ck::TReadWrite<FFragment_AStar_Params>,
	ck::TReadWrite<FFragment_Goap_Tier_Current>,
	ck::TReadWrite<FFragment_Goap_Tier_Actions>,
	ck::TReadOnly<FFragment_Goap_Tier_Requests>,
	ck::TReadWrite<FFragment_Goap_Tier_SearchState>,
	ck::TReadWrite<FFragment_Goap_Tier_Result>,
	ck::TReadWrite<FFragment_Goap_Tier_PlanContext>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<
		FProcessor_Goap_Tier_Setup,
		FProcessor_Goap_Tier_AutoReplan,
		FProcessor_Goap_WorldState_HandleRequests>;
	using MarkedDirtyBy = FFragment_Goap_Tier_Requests;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Tier_Current& InCurrent,
		FFragment_Goap_Tier_Actions& InActions,
		const FFragment_Goap_Tier_Requests& InRequests,
		FFragment_Goap_Tier_SearchState& InSearchState,
		FFragment_Goap_Tier_Result& InResult,
		FFragment_Goap_Tier_PlanContext& InPlanContext) const -> void;
};

// ====================================================================================================================
// EXECUTE — Time-sliced A* search (parallel; pure data)
// ====================================================================================================================

struct FProcessor_Goap_Tier_Execute
	: TProcessor_AStar_Execute<FProcessor_Goap_Tier_Execute,
		FCk_Handle_Goap_Tier, FFragment_Goap_Tier_SearchState, FFragment_Goap_Tier_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Tier_HandleRequests>;
};

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Tier_HandleResult : public ck_exp::TProcessor<
	FProcessor_Goap_Tier_HandleResult,
	FCk_Handle_Goap_Tier,
	ck::TReadOnly<FFragment_Goap_Tier_Result>,
	ck::TReadOnly<FFragment_Goap_Tier_PlanContext>,
	ck::TReadWrite<FFragment_Goap_Tier_Current>,
	FTag_AStar_SearchComplete,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Tier_Execute>;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Result& InResult,
		const FFragment_Goap_Tier_PlanContext& InPlanContext,
		FFragment_Goap_Tier_Current& InCurrent) -> void;
};

// ====================================================================================================================
// END PLAY — Clean up A* search state on entity destruction
// ====================================================================================================================

struct FProcessor_Goap_Tier_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_Goap_Tier_EndPlay,
		FCk_Handle_Goap_Tier, FFragment_Goap_Tier_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_EndPlay;
};

// ====================================================================================================================

} // namespace ck
