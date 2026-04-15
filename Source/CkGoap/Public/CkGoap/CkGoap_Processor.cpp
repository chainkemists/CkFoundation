#include "CkGoap_Processor.h"

#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/EntityScripts/CkGoapGoal_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_EndPlay);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — Extract CDO metadata into lightweight fragments
// ====================================================================================================================

auto
	FProcessor_Goap_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionClasses& InActionClasses,
		const FFragment_Goap_GoalClasses& InGoalClasses,
		FFragment_Goap_Actions& InActions,
		FFragment_Goap_Goals& InGoals)
	-> void
{
	InHandle.Remove<FTag_Goap_RequiresSetup>();

	// Extract action metadata from CDOs
	InActions._ActionDefs.Reset();
	InActions._ActionDefs.Reserve(InActionClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InActionClasses._Classes.Num(); ++Index)
	{
		const auto ActionClass = InActionClasses._Classes[Index];
		if (NOT ck::IsValid(ActionClass))
		{
			continue;
		}

		auto* CDO = ActionClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO))
		{
			continue;
		}

		CDO->DefineAction();

		auto ActionDef = goap::FActionDef{};
		ActionDef.ActionIndex = Index;
		ActionDef.Preconditions = CDO->_Preconditions;
		ActionDef.Effects = CDO->_Effects;
		ActionDef.Cost = CDO->_Cost;
		ActionDef.ActionClass = ActionClass;

		InActions._ActionDefs.Add(MoveTemp(ActionDef));
	}

	// Extract goal metadata from CDOs
	InGoals._GoalDefs.Reset();
	InGoals._GoalDefs.Reserve(InGoalClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InGoalClasses._Classes.Num(); ++Index)
	{
		const auto GoalClass = InGoalClasses._Classes[Index];
		if (NOT ck::IsValid(GoalClass))
		{
			continue;
		}

		auto* CDO = GoalClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO))
		{
			continue;
		}

		CDO->DefineGoal();

		auto GoalDef = goap::FGoalDef{};
		GoalDef.GoalIndex = Index;
		GoalDef.Conditions = CDO->_Conditions;
		GoalDef.Priority = CDO->_Priority;
		GoalDef.GoalClass = GoalClass;

		InGoals._GoalDefs.Add(MoveTemp(GoalDef));
	}
}

// ====================================================================================================================
// HANDLE REQUESTS
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
		FFragment_Goap_Current& InCurrent,
		const FFragment_Goap_Requests& InRequests,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext) const
	-> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Plan>)
			{
				DoHandleRequest(InHandle, InActions, InGoals, InWorldState, InCurrent,
					InSearchState, InResult, InPlanContext, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetWorldState>)
			{
				DoHandleRequest(InHandle, InWorldState, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_CancelPlan>)
			{
				DoHandleRequest(InHandle, InCurrent, InSearchState, InTypedRequest);
			}
		}));
	});
}

// ====================================================================================================================
// PLAN REQUEST — Select goal, build graph, start A* search
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		const FCk_Request_Goap_Plan& InRequest)
	-> void
{
	// Cancel any in-progress search
	InHandle.Try_Remove<FTag_AStar_SearchActive>();
	InHandle.Try_Remove<FTag_AStar_SearchComplete>();

	// Select goal
	const goap::FGoalDef* SelectedGoal = nullptr;

	if (ck::IsValid(InRequest.Get_SpecificGoalClass()))
	{
		// Specific goal requested
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.GoalClass == InRequest.Get_SpecificGoalClass())
			{
				SelectedGoal = &Goal;
				break;
			}
		}
	}
	else
	{
		// Auto-select highest priority goal whose conditions are not already satisfied
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.Conditions.IsSatisfiedBy(InWorldState._WorldState))
			{
				continue;
			}

			if (SelectedGoal == nullptr || Goal.Priority > SelectedGoal->Priority)
			{
				SelectedGoal = &Goal;
			}
		}
	}

	if (SelectedGoal == nullptr)
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;
		InCurrent._ActiveGoalClass = nullptr;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		return;
	}

	InCurrent._ActiveGoalClass = SelectedGoal->GoalClass;
	InCurrent._PlanStatus = ECk_GoapPlanStatus::Planning;
	InCurrent._Plan.Reset();
	InCurrent._PlanCost = 0.0f;

	// Build the GOAP graph adapter
	auto Graph = goap::FGoapGraph{
		InWorldState._WorldState,
		InActions._ActionDefs,
		SelectedGoal->Conditions};

	// Store graph in PlanContext (shares _Shared data with the copy inside TSearchState)
	InPlanContext._Graph = Graph;

	// Construct search state: start = goal conditions (index 0), goal = sentinel
	constexpr auto GoalSentinel = TNumericLimits<int32>::Max();
	InSearchState._State = astar::TSearchState<int32, goap::FGoapGraph>{
		MoveTemp(Graph),
		0,
		GoalSentinel};

	// Reset result
	InResult._Path.Reset();
	InResult._TotalCost = 0.0f;
	InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
	InResult._TotalIterations = 0;
	InResult._TotalTimeMicroseconds = 0;

	// Activate A* search
	InHandle.Add<FTag_AStar_SearchActive>();
}

// ====================================================================================================================
// SET WORLD STATE REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState& InWorldState,
		const FCk_Request_Goap_SetWorldState& InRequest)
	-> void
{
	InWorldState._WorldState.Set(InRequest.Get_Key(), InRequest.Get_Value());
}

// ====================================================================================================================
// CANCEL PLAN REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		const FCk_Request_Goap_CancelPlan& InRequest)
	-> void
{
	InHandle.Try_Remove<FTag_AStar_SearchActive>();
	InHandle.Try_Remove<FTag_AStar_SearchComplete>();

	InSearchState._State = {};
	InCurrent._PlanStatus = ECk_GoapPlanStatus::Idle;
	InCurrent._Plan.Reset();
	InCurrent._PlanCost = 0.0f;
	InCurrent._ActiveGoalClass = nullptr;
}

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
// ====================================================================================================================

auto
	FProcessor_Goap_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Result& InResult,
		const FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Current& InCurrent)
	-> void
{
	InHandle.Remove<FTag_AStar_SearchComplete>();

	const auto& Graph = InPlanContext._Graph;

	switch (InResult._SearchStatus)
	{
	case ECk_AStarSearchStatus::Complete:
	{
		// Convert path (state indices) to action sequence.
		// Path goes: [goalConditions(0), ..., satisfiedByCurrentState(N)]
		// Each edge represents an action applied in reverse.
		// Execution order = edges in REVERSE.

		const auto& Path = InResult._Path;
		auto ActionClasses = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};

		if (Path.Num() > 1)
		{
			ActionClasses.Reserve(Path.Num() - 1);

			for (auto Index = int32{0}; Index < Path.Num() - 1; ++Index)
			{
				const auto ActionIndex = Graph.Get_ActionForEdge(Path[Index], Path[Index + 1]);
				if (ActionIndex != INDEX_NONE)
				{
					const auto& Actions = Graph.Get_Actions();
					if (Actions.IsValidIndex(ActionIndex))
					{
						ActionClasses.Add(Actions[ActionIndex].ActionClass);
					}
				}
			}

			// Reverse for forward execution order
			Algo::Reverse(ActionClasses);
		}

		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
		InCurrent._Plan = ActionClasses;
		InCurrent._PlanCost = InResult._TotalCost;

		UUtils_Signal_OnGoapPlanComplete::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
				MoveTemp(ActionClasses), InResult._TotalCost}));
		break;
	}

	case ECk_AStarSearchStatus::Failed:
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		break;
	}

	case ECk_AStarSearchStatus::CostThresholdReached:
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		break;
	}

	default:
		break;
	}
}

// ====================================================================================================================

} // namespace ck
