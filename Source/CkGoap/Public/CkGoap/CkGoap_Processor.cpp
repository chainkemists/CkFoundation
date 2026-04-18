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
		const FFragment_Goap_ActionClasses& InActionClasses,
		const FFragment_Goap_GoalClasses& InGoalClasses,
		FFragment_Goap_KeyRegistry& InKeyRegistry,
		FFragment_Goap_Actions& InActions,
		FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Diagnostics& InDiagnostics)
	-> void
{
	InHandle.Remove<FTag_Goap_RequiresSetup>();

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

	// -- Phase 2: build the key registry by scanning every referenced tag ---
	// Do NOT reset the registry here. Gyms seed world state in DoConstruct
	// (a frame before Setup runs), which grows the registry on-demand via
	// FindOrRegister in CkGoap_Utils::DoSetTyped. Resetting would orphan
	// those WorldState slots (values sit at stale indices while the registry
	// renumbers tags in CDO-iteration order). Additive registration preserves
	// the gym's pre-seeded indices and lets Setup add any tags that only
	// appear in CDOs but weren't seeded.
	for (const auto& Entry : RawActions)
	{
		for (const auto& Pre : Entry.Preconditions) { InKeyRegistry._Registry.FindOrRegister(Pre.Key); }
		for (const auto& Eff : Entry.Effects)       { InKeyRegistry._Registry.FindOrRegister(Eff.Key); }
	}
	for (const auto& Entry : RawGoals)
	{
		for (const auto& C : Entry.Conditions) { InKeyRegistry._Registry.FindOrRegister(C.Key); }
	}

	if (InKeyRegistry._Registry.Num() > goap::WorldState_MaxKeys)
	{
		ck::goap::Warning(TEXT("GOAP [{}] has more distinct world-state keys ({}) than MAX_KEYS ({}). Excess keys will be rejected."),
			InHandle, InKeyRegistry._Registry.Num(), goap::WorldState_MaxKeys);
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
			const auto Resolved = ResolveCondition(InKeyRegistry._Registry, Pre);
			if (Resolved.IsValid()) { Def.Preconditions.Add(Resolved); }
		}
		for (const auto& Eff : Raw.Effects)
		{
			const auto Resolved = ResolveEffect(InKeyRegistry._Registry, Eff);
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
			const auto Resolved = ResolveCondition(InKeyRegistry._Registry, C);
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
		const FFragment_Goap_KeyRegistry& InKeyRegistry,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
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
				DoHandleRequest(InHandle, InKeyRegistry, InActions, InGoals, InWorldState, InCurrent,
					InSearchState, InResult, InPlanContext, InDiagnostics, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetWorldState>)
			{
				DoHandleRequest(InHandle, InKeyRegistry, InWorldState, InTypedRequest);
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
		const FFragment_Goap_KeyRegistry& InKeyRegistry,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
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

	// -- Goal selection ------------------------------------------------------
	const auto IsGoalSatisfied = [&](const goap::FGoalDef& InGoal) -> bool
	{
		for (const auto& C : InGoal.Conditions)
		{
			if (NOT InWorldState._WorldState.Satisfies(C)) { return false; }
		}
		return true;
	};

	const goap::FGoalDef* SelectedGoal = nullptr;

	if (ck::IsValid(InRequest.Get_SpecificGoalClass()))
	{
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.GoalClass == InRequest.Get_SpecificGoalClass()) { SelectedGoal = &Goal; break; }
		}
	}
	else
	{
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (IsGoalSatisfied(Goal)) { continue; }
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

	// -- Build graph and launch search --------------------------------------
	const auto GoalConditions = BuildConstraintSet(InKeyRegistry._Registry, SelectedGoal->Conditions);

	auto Graph = goap::FGoapGraph{
		InWorldState._WorldState,
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
// SET WORLD STATE REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		const FFragment_Goap_KeyRegistry& InKeyRegistry,
		FFragment_Goap_WorldState& InWorldState,
		const FCk_Request_Goap_SetWorldState& InRequest)
	-> void
{
	const auto Key = InKeyRegistry._Registry.Find(InRequest.Get_Key());
	if (Key == goap::InvalidGoapKey) { return; }

	InWorldState._WorldState.Set(Key, InRequest.Get_Value());
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

} // namespace ck
