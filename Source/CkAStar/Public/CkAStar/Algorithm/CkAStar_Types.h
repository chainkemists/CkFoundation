#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

enum class ESearchStatus : uint8
{
	InProgress,
	Complete,
	Failed,
	CostThresholdReached
};

// --------------------------------------------------------------------------------------------------------------------

struct FSearchParams
{
	// Time budget in microseconds. 0 = unlimited (run to completion).
	int64 BudgetMicroseconds = 0;

	// Max A* iterations per tick. 0 = unlimited (rely on time budget).
	int32 MaxIterationsPerTick = 0;

	// 0 = off; otherwise the search returns CostThresholdReached once the best FScore exceeds this.
	float CostThreshold = 0.0f;

	// Iterations between FPlatformTime samples: higher trades budget precision for less overhead.
	int32 TimecheckInterval = 16;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_NodeId>
struct TOpenSetEntry
{
	T_NodeId Node;
	float FScore = 0.0f;

	auto
	operator<(const TOpenSetEntry& InOther) const -> bool
	{
		return FScore < InOther.FScore;
	}
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_NodeId>
struct TSearchResult
{
	TArray<T_NodeId> Path;
	float TotalCost = 0.0f;
	ESearchStatus Status = ESearchStatus::Failed;
	int32 TotalIterations = 0;
	int64 TotalTimeMicroseconds = 0;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar
