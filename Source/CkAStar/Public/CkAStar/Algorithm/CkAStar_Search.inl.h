#pragma once

#include "CkAStar/CkAStar_Stats.h"

#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("AStar::ContinueSearch"), STAT_AStar_ContinueSearch, STATGROUP_CkAStar);
DECLARE_DWORD_COUNTER_STAT(TEXT("AStar Iterations"), STAT_AStar_Iterations, STATGROUP_CkAStar);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	ValidateExistingPath(
		const T_Graph& InGraph,
		const TArray<T_NodeId>& InPath)
	-> int32
{
	for (auto StepIndex = 0; StepIndex < InPath.Num() - 1; ++StepIndex)
	{
		const auto& Current = InPath[StepIndex];
		const auto& Next = InPath[StepIndex + 1];

		auto FoundNeighbor = false;
		for (const auto& Neighbor : InGraph.Neighbors(Current))
		{
			if (Neighbor == Next)
			{
				FoundNeighbor = true;
				break;
			}
		}

		if (NOT FoundNeighbor)
		{
			return StepIndex;
		}
	}

	return InPath.Num();
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
TSearchState<T_NodeId, T_Graph>::TSearchState(
	T_Graph InGraph,
	T_NodeId InStart,
	T_NodeId InGoal,
	int32 InInitialCapacity)
	: _Graph{MoveTemp(InGraph)}
	, _Start{MoveTemp(InStart)}
	, _Goal{MoveTemp(InGoal)}
	, _Status{ESearchStatus::InProgress}
{
	_OpenSet.Reserve(InInitialCapacity);
	_ClosedSet.Reserve(InInitialCapacity);
	_GScores.Reserve(InInitialCapacity);
	_CameFrom.Reserve(InInitialCapacity);

	SeedFreshSearch();
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
TSearchState<T_NodeId, T_Graph>::TSearchState(
	T_Graph InGraph,
	T_NodeId InStart,
	T_NodeId InGoal,
	const TArray<T_NodeId>& InExistingPath,
	int32 InWarmStartFromIndex,
	int32 InInitialCapacity)
	: _Graph{MoveTemp(InGraph)}
	, _Start{MoveTemp(InStart)}
	, _Goal{MoveTemp(InGoal)}
	, _Status{ESearchStatus::InProgress}
{
	_OpenSet.Reserve(InInitialCapacity);
	_ClosedSet.Reserve(InInitialCapacity);
	_GScores.Reserve(InInitialCapacity);
	_CameFrom.Reserve(InInitialCapacity);

	auto AccumulatedCost = 0.0f;

	for (auto PathIndex = 0; PathIndex < InWarmStartFromIndex; ++PathIndex)
	{
		const auto& Node = InExistingPath[PathIndex];

		_GScores.Add(Node, AccumulatedCost);
		_ClosedSet.Add(Node);

		if (PathIndex > 0)
		{
			_CameFrom.Add(Node, InExistingPath[PathIndex - 1]);
		}

		if (PathIndex < InWarmStartFromIndex - 1)
		{
			AccumulatedCost += _Graph.Cost(Node, InExistingPath[PathIndex + 1]);
		}
	}

	const auto& LastValidNode = InExistingPath[InWarmStartFromIndex - 1];
	_ClosedSet.Remove(LastValidNode);

	const auto HeuristicCost = _Graph.Heuristic(LastValidNode, _Goal);
	_OpenSet.HeapPush(TOpenSetEntry<T_NodeId>{LastValidNode, AccumulatedCost + HeuristicCost}, TLess<>{});
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TSearchState<T_NodeId, T_Graph>::ContinueSearch(
		const FSearchParams& InParams)
	-> ESearchStatus
{
	if (_Status != ESearchStatus::InProgress)
	{
		return _Status;
	}

	// Thread-safe — TProcessor_AStar_Execute runs parallel, so this scope shows up on worker threads.
	SCOPE_CYCLE_COUNTER(STAT_AStar_ContinueSearch);

	const auto StartCycles = FPlatformTime::Cycles64();
	auto IterationsThisTick = int32{0};

	const auto HasBudget = InParams.BudgetMicroseconds > 0;
	const auto HasMaxIterations = InParams.MaxIterationsPerTick > 0;
	const auto HasCostThreshold = InParams.CostThreshold > 0.0f;
	const auto CheckInterval = FMath::Max(InParams.TimecheckInterval, 1);

	while (_OpenSet.Num() > 0)
	{
		if (HasBudget && IterationsThisTick > 0 && (IterationsThisTick % CheckInterval == 0))
		{
			const auto NowCycles = FPlatformTime::Cycles64();
			const auto ElapsedMicroseconds =
				static_cast<int64>(FPlatformTime::ToSeconds64(NowCycles - StartCycles) * 1'000'000.0);

			if (ElapsedMicroseconds >= InParams.BudgetMicroseconds)
			{
				_TotalTimeMicroseconds += ElapsedMicroseconds;
				_TotalIterations += IterationsThisTick;
				return _Status; // remains InProgress
			}
		}

		if (HasMaxIterations && IterationsThisTick >= InParams.MaxIterationsPerTick)
		{
			const auto NowCycles = FPlatformTime::Cycles64();
			const auto ElapsedMicroseconds =
				static_cast<int64>(FPlatformTime::ToSeconds64(NowCycles - StartCycles) * 1'000'000.0);

			_TotalTimeMicroseconds += ElapsedMicroseconds;
			_TotalIterations += IterationsThisTick;
			return _Status; // remains InProgress
		}

		auto Entry = TOpenSetEntry<T_NodeId>{};
		_OpenSet.HeapPop(Entry, TLess<>{});

		const auto& Current = Entry.Node;

		// The heap can hold stale duplicates of a node that was closed after being pushed.
		if (_ClosedSet.Contains(Current))
		{
			continue;
		}

		if (_Graph.IsGoal(Current))
		{
			ReconstructPath(Current);
			_ResultCost = _GScores.FindChecked(Current);

			const auto NowCycles = FPlatformTime::Cycles64();
			_TotalTimeMicroseconds +=
				static_cast<int64>(FPlatformTime::ToSeconds64(NowCycles - StartCycles) * 1'000'000.0);
			_TotalIterations += IterationsThisTick;

			_Status = ESearchStatus::Complete;
			return _Status;
		}

		if (HasCostThreshold && Entry.FScore > InParams.CostThreshold)
		{
			const auto NowCycles = FPlatformTime::Cycles64();
			_TotalTimeMicroseconds +=
				static_cast<int64>(FPlatformTime::ToSeconds64(NowCycles - StartCycles) * 1'000'000.0);
			_TotalIterations += IterationsThisTick;

			_Status = ESearchStatus::CostThresholdReached;
			return _Status;
		}

		_ClosedSet.Add(Current);
		++IterationsThisTick;
		INC_DWORD_STAT(STAT_AStar_Iterations);

		const auto CurrentG = _GScores.FindChecked(Current);

		for (const auto& Neighbor : _Graph.Neighbors(Current))
		{
			if (_ClosedSet.Contains(Neighbor))
			{
				continue;
			}

			const auto TentativeG = CurrentG + _Graph.Cost(Current, Neighbor);

			const auto ExistingG = _GScores.Find(Neighbor);
			if (ExistingG != nullptr && TentativeG >= *ExistingG)
			{
				continue;
			}

			_GScores.Add(Neighbor, TentativeG);
			_CameFrom.Add(Neighbor, Current);

			const auto HeuristicCost = _Graph.Heuristic(Neighbor, _Goal);
			_OpenSet.HeapPush(
				TOpenSetEntry<T_NodeId>{Neighbor, TentativeG + HeuristicCost},
				TLess<>{});
		}
	}

	const auto NowCycles = FPlatformTime::Cycles64();
	_TotalTimeMicroseconds +=
		static_cast<int64>(FPlatformTime::ToSeconds64(NowCycles - StartCycles) * 1'000'000.0);
	_TotalIterations += IterationsThisTick;

	_Status = ESearchStatus::Failed;
	return _Status;
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TSearchState<T_NodeId, T_Graph>::GetResult() const
	-> TSearchResult<T_NodeId>
{
	return TSearchResult<T_NodeId>
	{
		.Path = _ResultPath,
		.TotalCost = _ResultCost,
		.Status = _Status,
		.TotalIterations = _TotalIterations,
		.TotalTimeMicroseconds = _TotalTimeMicroseconds
	};
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TSearchState<T_NodeId, T_Graph>::Reset(
		T_NodeId InNewStart,
		T_NodeId InNewGoal)
	-> void
{
	_Start = MoveTemp(InNewStart);
	_Goal = MoveTemp(InNewGoal);

	_OpenSet.Reset();
	_ClosedSet.Reset();
	_GScores.Reset();
	_CameFrom.Reset();
	_ResultPath.Reset();

	_ResultCost = 0.0f;
	_Status = ESearchStatus::InProgress;
	_TotalIterations = 0;
	_TotalTimeMicroseconds = 0;

	SeedFreshSearch();
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TSearchState<T_NodeId, T_Graph>::ReconstructPath(
		const T_NodeId& InGoalNode)
	-> void
{
	_ResultPath.Reset();

	auto Current = InGoalNode;
	_ResultPath.Add(Current);

	while (const auto* Parent = _CameFrom.Find(Current))
	{
		Current = *Parent;
		_ResultPath.Add(Current);
	}

	Algo::Reverse(_ResultPath);
}

// --------------------------------------------------------------------------------------------------------------------

template <AStarNodeId T_NodeId, typename T_Graph>
	requires AStarGraph<T_Graph, T_NodeId>
auto
	TSearchState<T_NodeId, T_Graph>::SeedFreshSearch()
	-> void
{
	_GScores.Add(_Start, 0.0f);

	const auto HeuristicCost = _Graph.Heuristic(_Start, _Goal);
	_OpenSet.HeapPush(TOpenSetEntry<T_NodeId>{_Start, HeuristicCost}, TLess<>{});
}

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar
