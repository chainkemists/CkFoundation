#pragma once

#include "CkAStar_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"

// ====================================================================================================================
//
// BASE TEMPLATE PROCESSORS — Consumers inherit from these, add Group/RunAfter, and register.
//
// Usage (e.g. in CkGoap):
//
//     // Define concrete fragment types
//     using FFragment_Goap_SearchState = ck::TFragment_AStar_SearchState<FGoapNodeId, FGoapGraph>;
//     using FFragment_Goap_Result = ck::TFragment_AStar_Result<FGoapNodeId>;
//
//     // Inherit from base processor, add scheduling
//     struct FProcessor_Goap_Execute
//         : ck::TProcessor_AStar_Execute<FProcessor_Goap_Execute,
//               FCk_Handle_Goap, FFragment_Goap_SearchState, FFragment_Goap_Result>
//     {
//         using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
//         using Group = FGroup_Gameplay_AI;
//         using RunAfter = TDepList<FProcessor_Goap_HandleRequests>;
//     };
//     CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Execute);
//
// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// EXECUTE PROCESSOR — Runs ContinueSearch on each active entity (parallel)
// ====================================================================================================================

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
		const auto SearchParams = InParams.ToSearchParams();
		const auto PreviousIterations = InSearchState._State.GetTotalIterations();

		const auto Status = InSearchState._State.ContinueSearch(SearchParams);

		// Write result
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

		// Update debug fragment (deferred because it may not exist on all entities)
		const auto BudgetUsage = (SearchParams.BudgetMicroseconds > 0)
			? static_cast<float>(InSearchState._State.GetTotalTimeMicroseconds())
				/ static_cast<float>(SearchParams.BudgetMicroseconds) * 100.0f
			: 0.0f;

		InHandle.DeferCustom([
			OpenSetSize = InSearchState._State.GetOpenSetSize(),
			ClosedSetSize = InSearchState._State.GetClosedSetSize(),
			IterationsThisFrame,
			TimeMicro = InSearchState._State.GetTotalTimeMicroseconds(),
			BudgetUsage,
			SearchStatus = InResult._SearchStatus
		](FCk_Handle& InDeferredHandle)
		{
			if (NOT InDeferredHandle.Has<FFragment_AStar_Debug>())
			{
				return;
			}

			InDeferredHandle.Get<FFragment_AStar_Debug>().ApplyUpdate(
				OpenSetSize, ClosedSetSize, IterationsThisFrame,
				TimeMicro, BudgetUsage, SearchStatus);
		});
	}
};

// ====================================================================================================================
// CLEANUP PROCESSOR — Resets search state when SearchComplete tag is consumed
// ====================================================================================================================

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
		InSearchState._State = {};
	}
};

// ====================================================================================================================

} // namespace ck
