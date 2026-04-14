#pragma once

#include "CkAStar_Types.h"
#include "CkAStar_GraphConcept.h"

// ====================================================================================================================

namespace ck::astar
{

// ====================================================================================================================
// VALIDATE EXISTING PATH — Plan repair: finds the first invalid step in a path
// ====================================================================================================================

// Walks the path and checks that each consecutive pair (Path[i], Path[i+1]) is connected
// via Graph.Neighbors(Path[i]). Returns the index of the first invalid step, or
// Path.Num() if the entire path is valid.

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
ValidateExistingPath(
	const T_Graph& InGraph,
	const TArray<T_NodeId>& InPath) -> int32;

// ====================================================================================================================
// SEARCH STATE — The core A* search, stored directly in consumer fragments
// ====================================================================================================================

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
struct TSearchState
{
public:
	// ----------------------------------------------------------------------------------------------------------------
	// CONSTRUCTION
	// ----------------------------------------------------------------------------------------------------------------

	// Fresh search: seeds open set with InStart, reserves containers to InInitialCapacity.
	TSearchState(
		T_Graph InGraph,
		T_NodeId InStart,
		T_NodeId InGoal,
		int32 InInitialCapacity = 64);

	// Warm-start (plan repair): seeds closed set and g-scores with the valid prefix
	// [0..InWarmStartFromIndex-1] of InExistingPath, opens search from the last valid node.
	TSearchState(
		T_Graph InGraph,
		T_NodeId InStart,
		T_NodeId InGoal,
		const TArray<T_NodeId>& InExistingPath,
		int32 InWarmStartFromIndex,
		int32 InInitialCapacity = 64);

	// Default-constructible for fragment storage. Must call Reset() or re-construct before use.
	TSearchState() = default;

	// ----------------------------------------------------------------------------------------------------------------
	// SEARCH EXECUTION
	// ----------------------------------------------------------------------------------------------------------------

	// Run A* iterations within the given budget. Returns the current status.
	// If InProgress, the state is preserved for the next call.
	auto
	ContinueSearch(const FSearchParams& InParams) -> ESearchStatus;

	// ----------------------------------------------------------------------------------------------------------------
	// RESULT ACCESS
	// ----------------------------------------------------------------------------------------------------------------

	auto
	GetStatus() const -> ESearchStatus { return _Status; }

	// Valid only when status is Complete.
	auto
	GetResultPath() const -> const TArray<T_NodeId>& { return _ResultPath; }

	auto
	GetResultCost() const -> float { return _ResultCost; }

	auto
	GetResult() const -> TSearchResult<T_NodeId>;

	// ----------------------------------------------------------------------------------------------------------------
	// DEBUG / STATS
	// ----------------------------------------------------------------------------------------------------------------

	auto
	GetOpenSetSize() const -> int32 { return _OpenSet.Num(); }

	auto
	GetClosedSetSize() const -> int32 { return _ClosedSet.Num(); }

	auto
	GetTotalIterations() const -> int32 { return _TotalIterations; }

	auto
	GetTotalTimeMicroseconds() const -> int64 { return _TotalTimeMicroseconds; }

	// ----------------------------------------------------------------------------------------------------------------
	// REUSE — Reset containers (preserving capacity) for a new search on the same graph
	// ----------------------------------------------------------------------------------------------------------------

	auto
	Reset(T_NodeId InNewStart, T_NodeId InNewGoal) -> void;

private:
	// ----------------------------------------------------------------------------------------------------------------
	// INTERNALS
	// ----------------------------------------------------------------------------------------------------------------

	auto
	ReconstructPath(const T_NodeId& InGoalNode) -> void;

	auto
	SeedFreshSearch() -> void;

	// ----------------------------------------------------------------------------------------------------------------
	// DATA
	// ----------------------------------------------------------------------------------------------------------------

	T_Graph _Graph{};
	T_NodeId _Start{};
	T_NodeId _Goal{};

	TArray<TOpenSetEntry<T_NodeId>> _OpenSet;
	TSet<T_NodeId> _ClosedSet;
	TMap<T_NodeId, float> _GScores;
	TMap<T_NodeId, T_NodeId> _CameFrom;

	TArray<T_NodeId> _ResultPath;
	float _ResultCost = 0.0f;

	ESearchStatus _Status = ESearchStatus::Failed;
	int32 _TotalIterations = 0;
	int64 _TotalTimeMicroseconds = 0;
};

// ====================================================================================================================

} // namespace ck::astar

// ====================================================================================================================

#include "CkAStar_Search.inl.h"
