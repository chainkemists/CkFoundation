#pragma once

#include "CkAStar_Search.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

// Build-time: runs full A* for each start-goal pair, stops when memory cap is reached.
// Query-time: hash lookup — instant.
// Serializable for cook-time precomputation.

template <AStarNodeId T_NodeId>
struct TPrecomputedTable
{
public:
	// ----------------------------------------------------------------------------------------------------------------

	struct FPrecomputedPath
	{
		TArray<T_NodeId> Path;
		float TotalCost = 0.0f;
	};

	struct FBuildStats
	{
		int32 PathsComputed = 0;
		int32 PathsSkipped = 0;
		int64 MemoryUsedBytes = 0;
		float MemoryUsedKB = 0.0f;
	};

	// ----------------------------------------------------------------------------------------------------------------

	// Iterates InStartGoalPairs in order, runs full A* (unlimited budget) for each,
	// stops when cumulative memory exceeds InMaxKB * 1024.
	template <typename T_Graph>
		requires AStarGraph<T_Graph, T_NodeId>
	auto
	Build(
		const T_Graph& InGraph,
		const TArray<TPair<T_NodeId, T_NodeId>>& InStartGoalPairs,
		float InMaxKB) -> FBuildStats;

	// ----------------------------------------------------------------------------------------------------------------

	// Hash lookup. Returns nullptr on miss.
	auto
	Find(const T_NodeId& InStart, const T_NodeId& InGoal) const -> const FPrecomputedPath*;

	// ----------------------------------------------------------------------------------------------------------------

	auto
	GetMemoryUsedBytes() const -> int64 { return _MemoryUsedBytes; }

	auto
	GetMemoryUsedKB() const -> float { return static_cast<float>(_MemoryUsedBytes) / 1024.0f; }

	auto
	GetPathCount() const -> int32 { return _Table.Num(); }

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Clear() -> void;

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Serialize(FArchive& InArchive) -> void;

private:
	// ----------------------------------------------------------------------------------------------------------------

	struct FKey
	{
		T_NodeId Start;
		T_NodeId Goal;

		auto
		operator==(const FKey& InOther) const -> bool
		{
			return Start == InOther.Start && Goal == InOther.Goal;
		}

		friend auto
		GetTypeHash(const FKey& InKey) -> uint32
		{
			return HashCombine(GetTypeHash(InKey.Start), GetTypeHash(InKey.Goal));
		}
	};

	// ----------------------------------------------------------------------------------------------------------------

	static auto
	EstimateEntryBytes(int32 InPathLength) -> int64;

	// ----------------------------------------------------------------------------------------------------------------

	TMap<FKey, FPrecomputedPath> _Table;
	int64 _MemoryUsedBytes = 0;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar

// --------------------------------------------------------------------------------------------------------------------

#include "CkAStar_PrecomputedTable.inl.h"
