#include "CkGoap_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/EntityScripts/CkGoapGoal_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_AutoReplan);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_EndPlay);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// LOCAL HELPERS — registry-driven resolution of raw (tag-keyed) conditions/effects
// ====================================================================================================================

namespace
{
	// Convert a raw (tag) condition into a keyed condition using the
	// registry. Returns an Invalid condition if the tag isn't registered.
	auto ResolveCondition(const goap::FKeyRegistry& InRegistry, const goap::FWorldStateCondition_Raw& InRaw)
		-> goap::FWorldStateCondition
	{
		const auto Key = InRegistry.Find(InRaw.Key);
		if (Key == goap::InvalidGoapKey) { return {}; }
		return goap::FWorldStateCondition{Key, InRaw.Value};
	}

	auto ResolveEffect(const goap::FKeyRegistry& InRegistry, const goap::FWorldStateEffect_Raw& InRaw)
		-> goap::FWorldStateEffect
	{
		const auto Key = InRegistry.Find(InRaw.Key);
		if (Key == goap::InvalidGoapKey) { return {}; }
		return goap::FWorldStateEffect{Key, InRaw.Value};
	}

	// Pack a goal's conditions into a FConstraintSet (the A*-side state).
	auto BuildConstraintSet(const goap::FKeyRegistry& InRegistry,
		const TArray<goap::FWorldStateCondition>& InConditions)
		-> goap::FConstraintSet
	{
		auto Set = goap::FConstraintSet{};
		for (const auto& C : InConditions)
		{
			if (C.IsValid()) { Set.Add(C); }
		}
		return Set;
	}
}

// ====================================================================================================================
// SETUP — Extract CDO metadata into lightweight fragments; build key registry
// ====================================================================================================================

auto
	FProcessor_Goap_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Params& InParams,
		const FFragment_Goap_ActionClasses& InActionClasses,
		const FFragment_Goap_GoalClasses& InGoalClasses,
		FFragment_Goap_Actions& InActions,
		FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Diagnostics& InDiagnostics)
	-> void
{
	InHandle.Remove<FTag_Goap_RequiresSetup>();

	auto Source = InParams.Get_WorldStateSource();
	CK_ENSURE_IF_NOT(ck::IsValid(Source),
		TEXT("GOAP planner [{}] Setup failed — _WorldStateSource is invalid (was the source destroyed before Setup ran?)."),
		InHandle)
	{ return; }

	auto& SourceRegistry = Source.Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

	// Local scratch structs — avoid the nested-TPair nightmare.
	struct FRawActionEntry
	{
		TSubclassOf<UCk_GoapAction_EntityScript> Class;
		TArray<goap::FWorldStateCondition_Raw>   Preconditions;
		TArray<goap::FWorldStateEffect_Raw>      Effects;
		float Cost = 1.0f;
	};
	struct FRawGoalEntry
	{
		TSubclassOf<UCk_GoapGoal_EntityScript>   Class;
		TArray<goap::FWorldStateCondition_Raw>   Conditions;
		int32 Priority = 0;
	};

	// -- Phase 1: pull the raw (tag-keyed) entries out of every CDO ----------
	auto RawActions = TArray<FRawActionEntry>{};
	RawActions.Reserve(InActionClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InActionClasses._Classes.Num(); ++Index)
	{
		const auto ActionClass = InActionClasses._Classes[Index];
		if (NOT ck::IsValid(ActionClass)) { continue; }
		auto* CDO = ActionClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO)) { continue; }
		CDO->DefineAction();

		auto Entry = FRawActionEntry{};
		Entry.Class         = ActionClass;
		Entry.Preconditions = CDO->_Preconditions;
		Entry.Effects       = CDO->_Effects;
		Entry.Cost          = CDO->_Cost;
		RawActions.Add(MoveTemp(Entry));
	}

	auto RawGoals = TArray<FRawGoalEntry>{};
	RawGoals.Reserve(InGoalClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InGoalClasses._Classes.Num(); ++Index)
	{
		const auto GoalClass = InGoalClasses._Classes[Index];
		if (NOT ck::IsValid(GoalClass)) { continue; }
		auto* CDO = GoalClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO)) { continue; }
		CDO->DefineGoal();

		auto Entry = FRawGoalEntry{};
		Entry.Class      = GoalClass;
		Entry.Conditions = CDO->_Conditions;
		Entry.Priority   = CDO->_Priority;
		RawGoals.Add(MoveTemp(Entry));
	}

	// -- Phase 2: register every referenced tag in the shared source registry.
	// Multiple planners pointing at the same source contribute their keys
	// additively — FindOrRegister is idempotent so the same tag from two
	// planners resolves to the same slot.
	for (const auto& Entry : RawActions)
	{
		for (const auto& Pre : Entry.Preconditions) { SourceRegistry.FindOrRegister(Pre.Key); }
		for (const auto& Eff : Entry.Effects)       { SourceRegistry.FindOrRegister(Eff.Key); }
	}
	for (const auto& Entry : RawGoals)
	{
		for (const auto& C : Entry.Conditions) { SourceRegistry.FindOrRegister(C.Key); }
	}

	if (SourceRegistry.Num() > goap::WorldState_MaxKeys)
	{
		ck::goap::Warning(TEXT("GOAP WorldState [{}] (source of planner [{}]) has more distinct keys ({}) than MAX_KEYS ({}). Excess keys will be rejected."),
			Source, InHandle, SourceRegistry.Num(), goap::WorldState_MaxKeys);
	}

	// -- Phase 3: resolve raw → typed ActionDef / GoalDef -------------------
	InActions._ActionDefs.Reset();
	InActions._ActionDefs.Reserve(RawActions.Num());

	for (auto Index = int32{0}; Index < RawActions.Num(); ++Index)
	{
		const auto& Raw = RawActions[Index];
		auto Def = goap::FActionDef{};
		Def.ActionIndex = Index;
		Def.ActionClass = Raw.Class;
		Def.Cost        = Raw.Cost;

		for (const auto& Pre : Raw.Preconditions)
		{
			const auto Resolved = ResolveCondition(SourceRegistry, Pre);
			if (Resolved.IsValid()) { Def.Preconditions.Add(Resolved); }
		}
		for (const auto& Eff : Raw.Effects)
		{
			const auto Resolved = ResolveEffect(SourceRegistry, Eff);
			if (Resolved.IsValid()) { Def.Effects.Add(Resolved); }
		}

		InActions._ActionDefs.Add(MoveTemp(Def));
	}

	InGoals._GoalDefs.Reset();
	InGoals._GoalDefs.Reserve(RawGoals.Num());

	for (auto Index = int32{0}; Index < RawGoals.Num(); ++Index)
	{
		const auto& Raw = RawGoals[Index];
		auto Def = goap::FGoalDef{};
		Def.GoalIndex = Index;
		Def.GoalClass = Raw.Class;
		Def.Priority  = Raw.Priority;

		for (const auto& C : Raw.Conditions)
		{
			const auto Resolved = ResolveCondition(SourceRegistry, C);
			if (Resolved.IsValid()) { Def.Conditions.Add(Resolved); }
		}

		InGoals._GoalDefs.Add(MoveTemp(Def));
	}

	// -- Phase 4: reset diagnostics -----------------------------------------
	// The boolean-era cycle detector doesn't translate cleanly to the numeric
	// domain — resources going up/down aren't "cycles" in the bad sense. A
	// future pass can implement numeric-aware analysis (e.g. "is every
	// constraint-kind reachable from initial values"). For now, keep the
	// cycle list empty so nothing spurious is flagged.
	InDiagnostics._DependencyCycles.Reset();
	InDiagnostics._LastUnreachableGoalConditions.Reset();
	InDiagnostics._LastFailedGoalClass = nullptr;
}

// ====================================================================================================================
// HANDLE REQUESTS
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Current& InCurrent,
		const FFragment_Goap_Requests& InRequests,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics) const
	-> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Plan>)
			{
				DoHandleRequest(InHandle, InParams, InActions, InGoals, InCurrent,
					InSearchState, InResult, InPlanContext, InDiagnostics, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_CancelPlan>)
			{
				DoHandleRequest(InHandle, InCurrent, InSearchState, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetActionCost>)
			{
				DoHandleRequest(InHandle, InActions, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetReplanInterval>)
			{
				DoHandleRequest(InHandle, InParams, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetReplanPolicy>)
			{
				DoHandleRequest(InHandle, InParams, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetSearchBudget>)
			{
				DoHandleRequest(InHandle, InParams, InAStarParams, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetCostThreshold>)
			{
				DoHandleRequest(InHandle, InParams, InAStarParams, InTypedRequest);
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
		const FFragment_Goap_Params& InParams,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics,
		const FCk_Request_Goap_Plan& InRequest)
	-> void
{
	InHandle.Try_Remove<FTag_AStar_SearchActive>();
	InHandle.Try_Remove<FTag_AStar_SearchComplete>();

	InDiagnostics._LastUnreachableGoalConditions.Reset();
	InDiagnostics._LastFailedGoalClass = nullptr;

	++InCurrent._PlanAttemptCount;

	auto Source = InParams.Get_WorldStateSource();
	if (NOT ck::IsValid(Source))
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;
		InCurrent._ActiveGoalClass = nullptr;
		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		return;
	}

	const auto& SourceWorldState = Source.Get<FFragment_Goap_WorldState_Values>().Get_Values();

	// -- Goal selection ------------------------------------------------------
	const auto IsGoalSatisfied = [&](const goap::FGoalDef& InGoal) -> bool
	{
		for (const auto& C : InGoal.Conditions)
		{
			if (NOT SourceWorldState.Satisfies(C)) { return false; }
		}
		return true;
	};

	const goap::FGoalDef* SelectedGoal = nullptr;
	auto bSelectedGoalAlreadySatisfied = false;

	if (ck::IsValid(InRequest.Get_SpecificGoalClass()))
	{
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.GoalClass == InRequest.Get_SpecificGoalClass()) { SelectedGoal = &Goal; break; }
		}
		if (SelectedGoal != nullptr && IsGoalSatisfied(*SelectedGoal))
		{
			bSelectedGoalAlreadySatisfied = true;
		}
	}
	else
	{
		// Prefer highest-priority UNSATISFIED goal so planning does useful work.
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (IsGoalSatisfied(Goal)) { continue; }
			if (SelectedGoal == nullptr || Goal.Priority > SelectedGoal->Priority)
			{
				SelectedGoal = &Goal;
			}
		}
		// All registered goals are already satisfied — planning trivially
		// succeeds with an empty action list. Report the highest-priority
		// goal as the one that was "reached" so the UI has something to show.
		if (SelectedGoal == nullptr)
		{
			for (const auto& Goal : InGoals._GoalDefs)
			{
				if (SelectedGoal == nullptr || Goal.Priority > SelectedGoal->Priority)
				{
					SelectedGoal = &Goal;
				}
			}
			if (SelectedGoal != nullptr) { bSelectedGoalAlreadySatisfied = true; }
		}
	}

	if (SelectedGoal == nullptr)
	{
		// No goals registered (or requested goal class not found) — genuine failure.
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;
		InCurrent._ActiveGoalClass = nullptr;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		return;
	}

	InCurrent._ActiveGoalClass = SelectedGoal->GoalClass;

	// Classical GOAP: a goal already satisfied by the current world state
	// means zero actions are needed — still a valid plan, just an empty one.
	if (bSelectedGoalAlreadySatisfied)
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;

		UUtils_Signal_OnGoapPlanComplete::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
				TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
		return;
	}

	InCurrent._PlanStatus = ECk_GoapPlanStatus::Planning;
	InCurrent._Plan.Reset();
	InCurrent._PlanCost = 0.0f;

	// -- Build graph and launch search --------------------------------------
	const auto& SourceRegistry = Source.Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
	const auto GoalConditions = BuildConstraintSet(SourceRegistry, SelectedGoal->Conditions);

	auto Graph = goap::FGoapGraph{
		SourceWorldState,
		InActions._ActionDefs,
		GoalConditions};

	InPlanContext._Graph = Graph;

	constexpr auto GoalSentinel = TNumericLimits<int32>::Max();
	InSearchState._State = astar::TSearchState<int32, goap::FGoapGraph>{
		MoveTemp(Graph),
		0,
		GoalSentinel};

	InResult._Path.Reset();
	InResult._TotalCost = 0.0f;
	InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
	InResult._TotalIterations = 0;
	InResult._TotalTimeMicroseconds = 0;

	InHandle.Add<FTag_AStar_SearchActive>();
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
// SET ACTION COST REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Actions& InActions,
		const FCk_Request_Goap_SetActionCost& InRequest)
	-> void
{
	const auto NewCost = InRequest.Get_NewCost();
	auto Changed = false;

	for (auto& ActionDef : InActions._ActionDefs)
	{
		if (ActionDef.ActionClass == InRequest.Get_ActionClass())
		{
			if (ActionDef.Cost != NewCost)
			{
				ActionDef.Cost = NewCost;
				Changed = true;
			}
			break;
		}
	}

	if (Changed)
	{
		InHandle.template AddOrGet<FTag_Goap_Dirty_Cost>();
	}
}

// ====================================================================================================================
// SET PARAM REQUESTS — Interval / Policy / Budget / CostThreshold
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		const FCk_Request_Goap_SetReplanInterval& InRequest)
	-> void
{
	InParams.Set_MinReplanIntervalSeconds(InRequest.Get_MinReplanIntervalSeconds());
}

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		const FCk_Request_Goap_SetReplanPolicy& InRequest)
	-> void
{
	InParams.Set_ReplanPolicy(InRequest.Get_ReplanPolicy());
}

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		const FCk_Request_Goap_SetSearchBudget& InRequest)
	-> void
{
	InParams.Set_SearchBudgetMicroseconds(InRequest.Get_SearchBudgetMicroseconds());
	InAStarParams.Set_BudgetMicroseconds(InRequest.Get_SearchBudgetMicroseconds());
}

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		const FCk_Request_Goap_SetCostThreshold& InRequest)
	-> void
{
	InParams.Set_CostThreshold(InRequest.Get_CostThreshold());
	InAStarParams.Set_CostThreshold(InRequest.Get_CostThreshold());
}

// ====================================================================================================================
// AUTO-REPLAN — Policy + throttle + initial-plan dispatch
// ====================================================================================================================

auto
	FProcessor_Goap_AutoReplan::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Params& InParams,
		FFragment_Goap_ReplanThrottle& InThrottle)
	-> void
{
	// Setup hasn't finished yet — world-state/cost writes haven't been
	// resolved against a valid key registry. Skip this tick.
	if (InHandle.Has<FTag_Goap_RequiresSetup>()) { return; }

	InThrottle._SecondsSinceLastReplan += InDeltaT.Get_Seconds();

	const auto IsInitialPlanPending = InHandle.Has<FTag_Goap_RequiresInitialPlan>();
	const auto WSDirty   = InHandle.Has<FTag_Goap_Dirty_WorldState>();
	const auto CostDirty = InHandle.Has<FTag_Goap_Dirty_Cost>();

	const auto PolicyAllowsReplan = [&]() -> bool
	{
		switch (InParams.Get_ReplanPolicy())
		{
			case ECk_Goap_ReplanPolicy::OnWorldStateDirty: return WSDirty;
			case ECk_Goap_ReplanPolicy::OnCostDirty:       return CostDirty;
			case ECk_Goap_ReplanPolicy::OnEitherDirty:     return WSDirty || CostDirty;
			case ECk_Goap_ReplanPolicy::Explicit:          return false;
		}
		return false;
	}();

	// Initial-plan fires unconditionally on the first post-setup tick; it
	// isn't throttled and isn't gated by policy.
	const auto ThrottleElapsed = InThrottle._SecondsSinceLastReplan
		>= InParams.Get_MinReplanIntervalSeconds();

	const auto ShouldFire = IsInitialPlanPending
		|| (PolicyAllowsReplan && ThrottleElapsed);

	if (NOT ShouldFire) { return; }

	// Enqueue a Plan request. HandleRequests runs after this processor in the
	// same frame (see RunAfter dependency), so the request is consumed
	// immediately. If a previous A* search is still active, the Plan handler
	// already performs CancelAndRestart via Try_Remove on the search tags.
	auto& Requests = InHandle.AddOrGet<FFragment_Goap_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Plan{});

	InThrottle._SecondsSinceLastReplan = 0.0f;
	InHandle.Try_Remove<FTag_Goap_Dirty_WorldState>();
	InHandle.Try_Remove<FTag_Goap_Dirty_Cost>();
	InHandle.Try_Remove<FTag_Goap_RequiresInitialPlan>();
}

// ====================================================================================================================

} // namespace ck
