#pragma once

#include "CkAStar_Fragment_Data.h"

#include "CkAStar/Algorithm/CkAStar_Types.h"
#include "CkAStar/Algorithm/CkAStar_Search.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// TAGS
// ====================================================================================================================

CK_DEFINE_ECS_TAG(FTag_AStar_SearchActive);
CK_DEFINE_ECS_TAG(FTag_AStar_SearchComplete);

// ====================================================================================================================
// PARAMS FRAGMENT — Non-template, shared config for any A* search
// ====================================================================================================================

struct CKASTAR_API FFragment_AStar_Params
{
public:
	CK_GENERATED_BODY(FFragment_AStar_Params);

	friend class UCk_Utils_AStar_UE;

private:
	int64 _BudgetMicroseconds = 500;
	int32 _MaxIterationsPerTick = 0;
	float _CostThreshold = 0.0f;
	int32 _TimecheckInterval = 16;

public:
	CK_PROPERTY(_BudgetMicroseconds);
	CK_PROPERTY(_MaxIterationsPerTick);
	CK_PROPERTY(_CostThreshold);
	CK_PROPERTY(_TimecheckInterval);

public:
	auto
	ToSearchParams() const -> astar::FSearchParams
	{
		return astar::FSearchParams
		{
			.BudgetMicroseconds = _BudgetMicroseconds,
			.MaxIterationsPerTick = _MaxIterationsPerTick,
			.CostThreshold = _CostThreshold,
			.TimecheckInterval = _TimecheckInterval
		};
	}
};

// ====================================================================================================================
// DEBUG FRAGMENT — Non-template, stats for the debugger
// ====================================================================================================================

struct CKASTAR_API FFragment_AStar_Debug
{
public:
	CK_GENERATED_BODY(FFragment_AStar_Debug);

	int32 _OpenSetSize = 0;
	int32 _ClosedSetSize = 0;
	int32 _IterationsThisFrame = 0;
	int64 _TimeThisFrameMicroseconds = 0;
	float _BudgetUsagePercent = 0.0f;
	ECk_AStarSearchStatus _SearchStatus = ECk_AStarSearchStatus::Idle;
};

// ====================================================================================================================
// SEARCH STATE FRAGMENT — Template, holds TSearchState directly (no indirection)
// ====================================================================================================================

template <astar::AStarNodeId T_NodeId, typename T_Graph>
	requires astar::AStarGraph<T_Graph, T_NodeId>
struct TFragment_AStar_SearchState
{
	using NodeIdType = T_NodeId;
	using GraphType = T_Graph;

	astar::TSearchState<T_NodeId, T_Graph> _State;
};

// ====================================================================================================================
// RESULT FRAGMENT — Template, holds the typed path
// ====================================================================================================================

template <astar::AStarNodeId T_NodeId>
struct TFragment_AStar_Result
{
	using NodeIdType = T_NodeId;

	TArray<T_NodeId> _Path;
	float _TotalCost = 0.0f;
	ECk_AStarSearchStatus _SearchStatus = ECk_AStarSearchStatus::Idle;
	int32 _TotalIterations = 0;
	int64 _TotalTimeMicroseconds = 0;
};

// ====================================================================================================================

} // namespace ck
