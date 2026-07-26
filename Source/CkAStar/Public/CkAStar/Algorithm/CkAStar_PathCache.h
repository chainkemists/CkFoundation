#pragma once

#include "CkAStar_GraphConcept.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

// Not thread-safe: read-only Find() during a parallel phase, Insert() deferred to the main thread.

template <AStarNodeId T_Key, AStarNodeId T_NodeId>
struct TPathCache
{
public:
	// ----------------------------------------------------------------------------------------------------------------

	struct FCachedPath
	{
		TArray<T_NodeId> Path;
		float TotalCost = 0.0f;
	};

	// ----------------------------------------------------------------------------------------------------------------

	explicit TPathCache(int32 InMaxEntries = 128);

	// ----------------------------------------------------------------------------------------------------------------

	// Mutating: nullptr on miss, a hit is promoted to the LRU front, a stale entry is evicted as a miss.
	auto
	Find(const T_Key& InKey) -> const FCachedPath*;

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Insert(T_Key InKey, TArray<T_NodeId> InPath, float InCost) -> void;

	// ----------------------------------------------------------------------------------------------------------------

	// Lazy: bumps the generation, frees nothing. Clear() is the hard reset.
	auto
	InvalidateAll() -> void;

	auto
	Clear() -> void;

	// ----------------------------------------------------------------------------------------------------------------

	auto
	GetEntryCount() const -> int32 { return _Entries.Num(); }

	auto
	GetGeneration() const -> uint32 { return _CurrentGeneration; }

	auto
	GetMaxEntries() const -> int32 { return _MaxEntries; }

private:
	// ----------------------------------------------------------------------------------------------------------------

	struct FEntry
	{
		T_Key Key;
		FCachedPath CachedPath;
		uint32 Generation = 0;
	};

	// ----------------------------------------------------------------------------------------------------------------

	TArray<FEntry> _Entries;
	int32 _MaxEntries = 128;
	uint32 _CurrentGeneration = 0;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar

// --------------------------------------------------------------------------------------------------------------------

#include "CkAStar_PathCache.inl.h"
