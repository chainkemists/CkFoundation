#pragma once

#include "CkAStar_Types.h"
#include "CkAStar_GraphConcept.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

// Returns the index of the first step (Path[i] -> Path[i+1]) not connected in the graph,
// or Path.Num() when the whole path is valid.
template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
ValidateExistingPath(
	const T_Graph& InGraph,
	const TArray<T_NodeId>& InPath) -> int32;

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
struct TSearchState
{
public:
	// ----------------------------------------------------------------------------------------------------------------

	TSearchState(
		T_Graph InGraph,
		T_NodeId InStart,
		T_NodeId InGoal,
		int32 InInitialCapacity = 64);

	// Warm-start (plan repair): InExistingPath[0..InWarmStartFromIndex-1] is taken as an
	// already-searched prefix; the search opens from the last node of that prefix.
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

	// Time-sliced: on InProgress the state is preserved for the next call.
	auto
	ContinueSearch(const FSearchParams& InParams) -> ESearchStatus;

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

	auto
	GetOpenSetSize() const -> int32 { return _OpenSet.Num(); }

	auto
	GetClosedSetSize() const -> int32 { return _ClosedSet.Num(); }

	auto
	GetTotalIterations() const -> int32 { return _TotalIterations; }

	auto
	GetTotalTimeMicroseconds() const -> int64 { return _TotalTimeMicroseconds; }

	auto
	GetClosedSet() const -> const TSet<T_NodeId>& { return _ClosedSet; }

	auto
	GetOpenSet() const -> const TArray<TOpenSetEntry<T_NodeId>>& { return _OpenSet; }

	auto
	GetGScores() const -> const TMap<T_NodeId, float>& { return _GScores; }

	auto
	GetCameFrom() const -> const TMap<T_NodeId, T_NodeId>& { return _CameFrom; }

	auto
	GetStart() const -> const T_NodeId& { return _Start; }

	auto
	GetGoal() const -> const T_NodeId& { return _Goal; }

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Reset(T_NodeId InNewStart, T_NodeId InNewGoal) -> void;

private:
	// ----------------------------------------------------------------------------------------------------------------

	auto
	ReconstructPath(const T_NodeId& InGoalNode) -> void;

	auto
	SeedFreshSearch() -> void;

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

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar

// --------------------------------------------------------------------------------------------------------------------

#include "CkAStar_Search.inl.h"
