#pragma once

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
TPathCache<T_Key, T_NodeId>::TPathCache(int32 InMaxEntries)
	: _MaxEntries{FMath::Max(InMaxEntries, 1)}
{
	_Entries.Reserve(_MaxEntries);
}

// --------------------------------------------------------------------------------------------------------------------

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

		if (Entry.Generation != _CurrentGeneration)
		{
			_Entries.RemoveAt(Index);
			return nullptr;
		}

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

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::Insert(
		T_Key InKey,
		TArray<T_NodeId> InPath,
		float InCost)
	-> void
{
	for (auto Index = 0; Index < _Entries.Num(); ++Index)
	{
		if (_Entries[Index].Key == InKey)
		{
			_Entries.RemoveAt(Index);
			break;
		}
	}

	while (_Entries.Num() >= _MaxEntries)
	{
		_Entries.RemoveAt(_Entries.Num() - 1);
	}

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

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::InvalidateAll()
	-> void
{
	++_CurrentGeneration;
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
auto
	TPathCache<T_Key, T_NodeId>::Clear()
	-> void
{
	_Entries.Empty();
	_CurrentGeneration = 0;
}

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar
