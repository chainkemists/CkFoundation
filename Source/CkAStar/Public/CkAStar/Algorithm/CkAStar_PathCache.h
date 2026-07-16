#pragma once

#include "CkAStar_GraphConcept.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

// Not thread-safe. Consumer is responsible for synchronization.
// Typical pattern: read-only Find() during parallel phase, Insert() deferred to main thread.

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

	// Returns nullptr on miss. On hit, promotes to front (LRU) and skips stale entries.
	auto
	Find(const T_Key& InKey) -> const FCachedPath*;

	// ----------------------------------------------------------------------------------------------------------------

	// Removes duplicate key if present, evicts LRU tail if at capacity, inserts at front.
	auto
	Insert(T_Key InKey, TArray<T_NodeId> InPath, float InCost) -> void;

	// ----------------------------------------------------------------------------------------------------------------

	// Increments generation counter. All existing entries become stale.
	auto
	InvalidateAll() -> void;

	// Hard clear — removes all entries.
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
