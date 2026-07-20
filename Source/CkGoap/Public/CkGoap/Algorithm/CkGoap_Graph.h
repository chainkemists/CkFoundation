#pragma once

#include "CkGoap_Types.h"
#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::goap
{

// --------------------------------------------------------------------------------------------------------------------
// Regressive planning graph adapter satisfying AStarGraph<FGoapGraph, int32>
//
// Nodes are FConstraintSets (the regressive-search state type — a set of
// typed constraints a predecessor state must satisfy). NodeId = int32 = index
// into _StatePool.
//
// Edges are actions applied in reverse: an action's effects resolve
// constraints (Set removes; Add/Sub shifts) and its preconditions become new
// constraints on the predecessor.
//
// Search direction: BACKWARD (regressive)
//   Start = the goal's conditions expressed as an FConstraintSet (index 0)
//   Goal  = "every constraint satisfied by the current world state"
//
// Plan extraction: reverse the edge actions along the A* path to get
// execution order.

struct FGoapGraphSharedData
{
	// Every regressive-search node discovered by Neighbors() lives here. The
	// A* closed set keys off the int32 index so two nodes with the same
	// content collapse via the StateLookup table below.
	TArray<FConstraintSet> StatePool;
	TMap<int64, int32> EdgeActions;

	// Content-addressed lookup — equivalent FConstraintSets reached via
	// different action orderings share a single StatePool index. Without
	// this, conjunctive goals explode the search space (every ordering
	// produces a unique index even though the content is identical).
	TMap<uint32, TArray<int32>> StateLookup;
};

// --------------------------------------------------------------------------------------------------------------------

struct FGoapGraph
{
public:
	FGoapGraph() = default;

	FGoapGraph(
		FWorldState InCurrentWorldState,
		TArray<FActionDef> InActions,
		const FConstraintSet& InGoalConditions)
		: _CurrentWorldState{MoveTemp(InCurrentWorldState)}
		, _Actions{MoveTemp(InActions)}
		, _Shared{MakeShared<FGoapGraphSharedData>()}
	{
		_Shared->StatePool.Reserve(64);
		_Shared->StatePool.Add(InGoalConditions);

		const auto InitialHash = InGoalConditions.GetHash();
		_Shared->StateLookup.FindOrAdd(InitialHash).Add(0);
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
	// ----------------------------------------------------------------------------------------------------------------

	// Returns the action index used to transition from InFrom to InTo, or INDEX_NONE.
	auto
	Get_ActionForEdge(int32 InFrom, int32 InTo) const -> int32;

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto
	Get_CurrentWorldState() const -> const FWorldState& { return _CurrentWorldState; }

	auto
	Get_Actions() const -> const TArray<FActionDef>& { return _Actions; }

	auto
	Get_StatePool() const -> const TArray<FConstraintSet>&
	{
		check(_Shared.IsValid());
		return _Shared->StatePool;
	}

	auto
	Get_StatePoolSize() const -> int32
	{
		return _Shared.IsValid() ? _Shared->StatePool.Num() : 0;
	}

	// Edge map (PackEdgeKey(from,to) → action index) — read by the debugger to
	// reconstruct which action's reverse application discovered each node.
	auto
	Get_EdgeActions() const -> const TMap<int64, int32>&
	{
		check(_Shared.IsValid());
		return _Shared->EdgeActions;
	}

private:
	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	static auto
	PackEdgeKey(int32 InFrom, int32 InTo) -> int64
	{
		return (static_cast<int64>(InFrom) << 32) | static_cast<int64>(static_cast<uint32>(InTo));
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	FWorldState _CurrentWorldState;
	TArray<FActionDef> _Actions;

	TSharedPtr<FGoapGraphSharedData> _Shared;
};

// --------------------------------------------------------------------------------------------------------------------

static_assert(
	astar::AStarGraph<FGoapGraph, int32>,
	"FGoapGraph must satisfy the AStarGraph concept");

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::goap

// --------------------------------------------------------------------------------------------------------------------

#include "CkGoap_Graph.inl.h"
