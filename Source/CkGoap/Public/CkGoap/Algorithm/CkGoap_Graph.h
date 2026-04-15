#pragma once

#include "CkGoap_Types.h"
#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

// ====================================================================================================================

namespace ck::goap
{

// ====================================================================================================================
// GOAP GRAPH — Regressive planning graph adapter satisfying AStarGraph<FGoapGraph, int32>
// ====================================================================================================================

// Nodes are partial world states (condition sets) stored in a pool.
// NodeId = int32 = index into _StatePool.
// Edges are actions applied in reverse: an action's effects satisfy conditions,
// and its preconditions become new requirements.
//
// Search direction: BACKWARD (regressive)
//   Start = goal conditions (index 0)
//   Goal  = "all conditions satisfied by current world state" (checked via IsGoal)
//
// Plan extraction: reverse the edge actions along the A* path to get execution order.

// Shared mutable data — survives graph copies (TSearchState stores the graph by value).
// When the A* search calls Neighbors() on its internal copy, edge and state data
// are written to these shared containers, readable by the HandleResult processor
// through the original FGoapGraph stored in FFragment_Goap_PlanContext.

struct FGoapGraphSharedData
{
	TArray<FWorldState> StatePool;
	TMap<int64, int32> EdgeActions;
};

// ====================================================================================================================

struct FGoapGraph
{
public:
	FGoapGraph() = default;

	FGoapGraph(
		FWorldState InCurrentWorldState,
		TArray<FActionDef> InActions,
		const FWorldState& InGoalConditions)
		: _CurrentWorldState{MoveTemp(InCurrentWorldState)}
		, _Actions{MoveTemp(InActions)}
		, _Shared{MakeShared<FGoapGraphSharedData>()}
	{
		_Shared->StatePool.Reserve(64);
		_Shared->StatePool.Add(InGoalConditions);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// AStarGraph INTERFACE
	// ----------------------------------------------------------------------------------------------------------------

	auto
	Neighbors(int32 InStateIndex) const -> TArray<int32>;

	auto
	Cost(int32 InFrom, int32 InTo) const -> float;

	auto
	Heuristic(int32 InCurrent, int32 InGoal) const -> float;

	auto
	IsGoal(int32 InStateIndex) const -> bool;

	// ----------------------------------------------------------------------------------------------------------------
	// PLAN EXTRACTION
	// ----------------------------------------------------------------------------------------------------------------

	// Returns the action index used to transition from InFrom to InTo, or INDEX_NONE.
	auto
	Get_ActionForEdge(int32 InFrom, int32 InTo) const -> int32;

	// ----------------------------------------------------------------------------------------------------------------
	// ACCESSORS
	// ----------------------------------------------------------------------------------------------------------------

	auto
	Get_CurrentWorldState() const -> const FWorldState& { return _CurrentWorldState; }

	auto
	Get_Actions() const -> const TArray<FActionDef>& { return _Actions; }

	auto
	Get_StatePool() const -> const TArray<FWorldState>&
	{
		check(_Shared.IsValid());
		return _Shared->StatePool;
	}

	auto
	Get_StatePoolSize() const -> int32
	{
		return _Shared.IsValid() ? _Shared->StatePool.Num() : 0;
	}

private:
	// ----------------------------------------------------------------------------------------------------------------
	// INTERNALS
	// ----------------------------------------------------------------------------------------------------------------

	static auto
	PackEdgeKey(int32 InFrom, int32 InTo) -> int64
	{
		return (static_cast<int64>(InFrom) << 32) | static_cast<int64>(static_cast<uint32>(InTo));
	}

	// ----------------------------------------------------------------------------------------------------------------
	// DATA
	// ----------------------------------------------------------------------------------------------------------------

	FWorldState _CurrentWorldState;
	TArray<FActionDef> _Actions;

	// Shared across graph copies — when TSearchState copies the graph,
	// both the copy and the original share this data.
	TSharedPtr<FGoapGraphSharedData> _Shared;
};

// ====================================================================================================================

// Static concept check
static_assert(
	astar::AStarGraph<FGoapGraph, int32>,
	"FGoapGraph must satisfy the AStarGraph concept");

// ====================================================================================================================

} // namespace ck::goap

// ====================================================================================================================

#include "CkGoap_Graph.inl.h"
