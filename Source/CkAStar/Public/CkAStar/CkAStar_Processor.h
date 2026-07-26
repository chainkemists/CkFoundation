#pragma once

#include "CkAStar_Fragment.h"
#include "CkAStar/CkAStar_Stats.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("AStar::Execute"), STAT_AStar_Execute, STATGROUP_CkAStar);
DECLARE_CYCLE_STAT(TEXT("AStar::EndPlay"), STAT_AStar_EndPlay, STATGROUP_CkAStar);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

template <
	typename T_DerivedProcessor,
	typename T_HandleType,
	typename T_SearchStateFragment,
	typename T_ResultFragment>
class TProcessor_AStar_Execute : public TParallelProcessor<
	T_DerivedProcessor,
	T_HandleType,
	TReadOnly<FFragment_AStar_Params>,
	TReadWrite<T_SearchStateFragment>,
	TReadWrite<T_ResultFragment>,
	FTag_AStar_SearchActive,
	CK_IGNORE_PENDING_KILL>
{
	using Super = TParallelProcessor<
		T_DerivedProcessor,
		T_HandleType,
		TReadOnly<FFragment_AStar_Params>,
		TReadWrite<T_SearchStateFragment>,
		TReadWrite<T_ResultFragment>,
		FTag_AStar_SearchActive,
		CK_IGNORE_PENDING_KILL>;

public:
	using Super::Super;

	static auto
	ForEachEntity(
		typename Super::TimeType InDeltaT,
		typename Super::HandleType InHandle,
		const FFragment_AStar_Params& InParams,
		T_SearchStateFragment& InSearchState,
		T_ResultFragment& InResult) -> void
	{
		SCOPE_CYCLE_COUNTER(STAT_AStar_Execute);

		const auto SearchParams = InParams.ToSearchParams();
		const auto PreviousIterations = InSearchState._State.GetTotalIterations();

		const auto Status = InSearchState._State.ContinueSearch(SearchParams);

		const auto IterationsThisFrame = InSearchState._State.GetTotalIterations() - PreviousIterations;

		InResult._TotalIterations = InSearchState._State.GetTotalIterations();
		InResult._TotalTimeMicroseconds = InSearchState._State.GetTotalTimeMicroseconds();

		switch (Status)
		{
		case astar::ESearchStatus::Complete:
			InResult._Path = InSearchState._State.GetResultPath();
			InResult._TotalCost = InSearchState._State.GetResultCost();
			InResult._SearchStatus = ECk_AStarSearchStatus::Complete;
			InHandle.template DeferTry_Remove<FTag_AStar_SearchActive>();
			InHandle.template DeferAdd<FTag_AStar_SearchComplete>();
			break;

		case astar::ESearchStatus::Failed:
			InResult._Path.Reset();
			InResult._TotalCost = 0.0f;
			InResult._SearchStatus = ECk_AStarSearchStatus::Failed;
			InHandle.template DeferTry_Remove<FTag_AStar_SearchActive>();
			InHandle.template DeferAdd<FTag_AStar_SearchComplete>();
			break;

		case astar::ESearchStatus::CostThresholdReached:
			InResult._Path.Reset();
			InResult._TotalCost = 0.0f;
			InResult._SearchStatus = ECk_AStarSearchStatus::CostThresholdReached;
			InHandle.template DeferTry_Remove<FTag_AStar_SearchActive>();
			InHandle.template DeferAdd<FTag_AStar_SearchComplete>();
			break;

		case astar::ESearchStatus::InProgress:
			InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
			break;
		}

		// Deferred — the debug fragment is optional and outside this parallel processor's access policy.
		const auto BudgetUsage = (SearchParams.BudgetMicroseconds > 0)
			? static_cast<float>(InSearchState._State.GetTotalTimeMicroseconds())
				/ static_cast<float>(SearchParams.BudgetMicroseconds) * 100.0f
			: 0.0f;

		const auto NewDebug = FFragment_AStar_Debug{
			InSearchState._State.GetOpenSetSize(),
			InSearchState._State.GetClosedSetSize(),
			IterationsThisFrame,
			InSearchState._State.GetTotalTimeMicroseconds(),
			BudgetUsage,
			InResult._SearchStatus};

		InHandle.DeferCustom([NewDebug](FCk_Handle& InDeferredHandle)
		{
			if (NOT InDeferredHandle.Has<FFragment_AStar_Debug>())
			{
				return;
			}

			InDeferredHandle.Get<FFragment_AStar_Debug>() = NewDebug;
		});
	}
};

// --------------------------------------------------------------------------------------------------------------------

template <
	typename T_DerivedProcessor,
	typename T_HandleType,
	typename T_SearchStateFragment>
class TProcessor_AStar_EndPlay : public ck_exp::TProcessor<
	T_DerivedProcessor,
	T_HandleType,
	TReadWrite<T_SearchStateFragment>,
	CK_IF_END_PLAY>
{
	using Super = ck_exp::TProcessor<
		T_DerivedProcessor,
		T_HandleType,
		TReadWrite<T_SearchStateFragment>,
		CK_IF_END_PLAY>;

public:
	using Super::Super;

	static auto
	ForEachEntity(
		typename Super::TimeType InDeltaT,
		typename Super::HandleType InHandle,
		T_SearchStateFragment& InSearchState) -> void
	{
		SCOPE_CYCLE_COUNTER(STAT_AStar_EndPlay);

		InSearchState._State = {};
	}
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
