#pragma once

// ====================================================================================================================

namespace ck::astar
{

// ====================================================================================================================
// CONSTRUCTION
// ====================================================================================================================

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
TPathCache<T_Key, T_NodeId>::TPathCache(int32 InMaxEntries)
	: _MaxEntries{FMath::Max(InMaxEntries, 1)}
{
	_Entries.Reserve(_MaxEntries);
}

// ====================================================================================================================
// FIND — LRU lookup with generation check
// ====================================================================================================================

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::Find(
		const T_Key& InKey)
	-> const FCachedPath*
{
	for (auto Index = 0; Index < _Entries.Num(); ++Index)
	{
		auto& Entry = _Entries[Index];

		if (NOT (Entry.Key == InKey))
		{
			continue;
		}

		// Stale entry — skip (effectively a miss)
		if (Entry.Generation != _CurrentGeneration)
		{
			_Entries.RemoveAt(Index);
			return nullptr;
		}

		// Hit — promote to front (LRU)
		if (Index > 0)
		{
			auto HitEntry = MoveTemp(Entry);
			_Entries.RemoveAt(Index);
			_Entries.Insert(MoveTemp(HitEntry), 0);
		}

		return &_Entries[0].CachedPath;
	}

	return nullptr;
}

// ====================================================================================================================
// INSERT
// ====================================================================================================================

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::Insert(
		T_Key InKey,
		TArray<T_NodeId> InPath,
		float InCost)
	-> void
{
	// Remove existing entry with same key
	for (auto Index = 0; Index < _Entries.Num(); ++Index)
	{
		if (_Entries[Index].Key == InKey)
		{
			_Entries.RemoveAt(Index);
			break;
		}
	}

	// Evict LRU tail if at capacity
	while (_Entries.Num() >= _MaxEntries)
	{
		_Entries.RemoveAt(_Entries.Num() - 1);
	}

	// Insert at front
	auto NewEntry = FEntry
	{
		.Key = MoveTemp(InKey),
		.CachedPath = FCachedPath
		{
			.Path = MoveTemp(InPath),
			.TotalCost = InCost
		},
		.Generation = _CurrentGeneration
	};

	_Entries.Insert(MoveTemp(NewEntry), 0);
}

// ====================================================================================================================
// INVALIDATION
// ====================================================================================================================

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::InvalidateAll()
	-> void
{
	++_CurrentGeneration;
}

// ====================================================================================================================
// CLEAR
// ====================================================================================================================

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::Clear()
	-> void
{
	_Entries.Empty();
	_CurrentGeneration = 0;
}

// ====================================================================================================================

} // namespace ck::astar
