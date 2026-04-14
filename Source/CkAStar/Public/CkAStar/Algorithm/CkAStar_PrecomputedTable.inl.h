#pragma once

// ====================================================================================================================

namespace ck::astar
{

// ====================================================================================================================
// BUILD — Precompute paths up to KB cap
// ====================================================================================================================

template <AStarNodeId T_NodeId>
template <typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TPrecomputedTable<T_NodeId>::Build(
		const T_Graph& InGraph,
		const TArray<TPair<T_NodeId, T_NodeId>>& InStartGoalPairs,
		float InMaxKB)
	-> FBuildStats
{
	const auto MaxBytes = static_cast<int64>(InMaxKB * 1024.0f);
	auto Stats = FBuildStats{};

	for (const auto& [Start, Goal] : InStartGoalPairs)
	{
		// Check if we already have this pair
		const auto Key = FKey{Start, Goal};
		if (_Table.Contains(Key))
		{
			++Stats.PathsSkipped;
			continue;
		}

		// Run full A* with unlimited budget
		auto SearchState = TSearchState<T_NodeId, T_Graph>{InGraph, Start, Goal};
		const auto UnlimitedParams = FSearchParams{};
		const auto Status = SearchState.ContinueSearch(UnlimitedParams);

		if (Status != ESearchStatus::Complete)
		{
			++Stats.PathsSkipped;
			continue;
		}

		const auto& ResultPath = SearchState.GetResultPath();
		const auto EntryBytes = EstimateEntryBytes(ResultPath.Num());

		// Check memory cap before inserting
		if (_MemoryUsedBytes + EntryBytes > MaxBytes)
		{
			++Stats.PathsSkipped;
			break;
		}

		// Insert
		auto PrecomputedPath = FPrecomputedPath
		{
			.Path = ResultPath,
			.TotalCost = SearchState.GetResultCost()
		};

		_Table.Add(Key, MoveTemp(PrecomputedPath));
		_MemoryUsedBytes += EntryBytes;
		++Stats.PathsComputed;
	}

	Stats.MemoryUsedBytes = _MemoryUsedBytes;
	Stats.MemoryUsedKB = static_cast<float>(_MemoryUsedBytes) / 1024.0f;

	return Stats;
}

// ====================================================================================================================
// FIND — Hash lookup
// ====================================================================================================================

template <AStarNodeId T_NodeId>
auto
	TPrecomputedTable<T_NodeId>::Find(
		const T_NodeId& InStart,
		const T_NodeId& InGoal) const
	-> const FPrecomputedPath*
{
	const auto Key = FKey{InStart, InGoal};
	return _Table.Find(Key);
}

// ====================================================================================================================
// CLEAR
// ====================================================================================================================

template <AStarNodeId T_NodeId>
auto
	TPrecomputedTable<T_NodeId>::Clear()
	-> void
{
	_Table.Empty();
	_MemoryUsedBytes = 0;
}

// ====================================================================================================================
// SERIALIZE
// ====================================================================================================================

template <AStarNodeId T_NodeId>
auto
	TPrecomputedTable<T_NodeId>::Serialize(
		FArchive& InArchive)
	-> void
{
	auto EntryCount = _Table.Num();
	InArchive << EntryCount;

	if (InArchive.IsLoading())
	{
		_Table.Empty();
		_Table.Reserve(EntryCount);
		_MemoryUsedBytes = 0;

		for (auto Index = 0; Index < EntryCount; ++Index)
		{
			auto Key = FKey{};
			auto Path = FPrecomputedPath{};

			InArchive << Key.Start;
			InArchive << Key.Goal;
			InArchive << Path.TotalCost;

			auto PathLength = int32{0};
			InArchive << PathLength;
			Path.Path.SetNum(PathLength);

			for (auto PathIndex = 0; PathIndex < PathLength; ++PathIndex)
			{
				InArchive << Path.Path[PathIndex];
			}

			_MemoryUsedBytes += EstimateEntryBytes(PathLength);
			_Table.Add(MoveTemp(Key), MoveTemp(Path));
		}
	}
	else
	{
		for (auto& [Key, Path] : _Table)
		{
			auto MutableKey = Key;
			InArchive << MutableKey.Start;
			InArchive << MutableKey.Goal;
			InArchive << Path.TotalCost;

			auto PathLength = Path.Path.Num();
			InArchive << PathLength;

			for (auto PathIndex = 0; PathIndex < PathLength; ++PathIndex)
			{
				InArchive << Path.Path[PathIndex];
			}
		}
	}
}

// ====================================================================================================================
// ESTIMATE ENTRY BYTES
// ====================================================================================================================

template <AStarNodeId T_NodeId>
auto
	TPrecomputedTable<T_NodeId>::EstimateEntryBytes(
		int32 InPathLength)
	-> int64
{
	constexpr auto KeyOverhead = static_cast<int64>(sizeof(FKey));
	constexpr auto PathStructOverhead = static_cast<int64>(sizeof(FPrecomputedPath));
	constexpr auto TMapEntryOverhead = int64{64};

	const auto PathDataBytes = static_cast<int64>(InPathLength) * static_cast<int64>(sizeof(T_NodeId));

	return KeyOverhead + PathStructOverhead + PathDataBytes + TMapEntryOverhead;
}

// ====================================================================================================================

} // namespace ck::astar
