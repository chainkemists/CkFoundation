#include "CkGoap/Tier/CkGoap_Tier_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_AutoReplan);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Tier_EndPlay);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// LOCAL HELPERS — registry-driven resolution of raw (tag-keyed) conditions/effects
// ====================================================================================================================

namespace
{
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

	auto BuildConstraintSet(const TArray<goap::FWorldStateCondition>& InConditions)
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
// SETUP — per-tier CDO extraction
// ====================================================================================================================

auto
	FProcessor_Goap_Tier_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Params& InParams,
		const FFragment_Goap_Tier_ActionClasses& InClasses,
		FFragment_Goap_Tier_Actions& InActions,
		FFragment_Goap_Tier_Current& InCurrent) -> void
{
	// Setup must wait for the WS source to be resolved. Root tiers get this
	// at AddTier time (if override is valid); non-root tiers get it at
	// activation time via ChainUpdate. If the resolved source is invalid,
	// defer to next frame WITHOUT removing the setup tag.
	const auto Source = InCurrent.Get_WorldStateSource_Resolved();
	if (NOT ck::IsValid(Source))
	{
		// Root with no override + no override-yet, or non-root waiting for
		// activation. Don't complain — this is the normal pre-activation
		// state for catalog-resident tiers.
		return;
	}

	InHandle.Remove<FTag_Goap_Tier_RequiresSetup>();

	auto& SourceRegistry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

	struct FRawActionEntry
	{
		TSubclassOf<UCk_GoapAction_EntityScript> Class;
		TArray<goap::FWorldStateCondition_Raw>   Preconditions;
		TArray<goap::FWorldStateEffect_Raw>      Effects;
		float                                    Cost = 1.0f;
		FGameplayTag                             ActionTag;
	};

	auto RawActions = TArray<FRawActionEntry>{};
	RawActions.Reserve(InClasses.Get_Classes().Num());

	for (const auto& ActionClass : InClasses.Get_Classes())
	{
		if (NOT ck::IsValid(ActionClass)) { continue; }
		auto* CDO = ActionClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO)) { continue; }
		CDO->DefineAction();

		auto Entry = FRawActionEntry{};
		Entry.Class         = ActionClass;
		Entry.Preconditions = CDO->_Preconditions;
		Entry.Effects       = CDO->_Effects;
		Entry.Cost          = CDO->_Cost;
		Entry.ActionTag     = CDO->Get_ActionTag();
		RawActions.Add(MoveTemp(Entry));
	}

	// Register every referenced tag in the shared source registry.
	for (const auto& Entry : RawActions)
	{
		for (const auto& Pre : Entry.Preconditions) { SourceRegistry.FindOrRegister(Pre.Key); }
		for (const auto& Eff : Entry.Effects)       { SourceRegistry.FindOrRegister(Eff.Key); }
	}

	// Register root-tier's initial-goal keys too (so the resolved goal hits
	// valid slots).
	for (const auto& Cond : InParams.Get_InitialGoal_RootOnly())
	{
		SourceRegistry.FindOrRegister(Cond.Get_Key());
	}

	if (SourceRegistry.Num() > goap::WorldState_MaxKeys)
	{
		ck::goap::Warning(
			TEXT("GOAP WorldState [{}] (resolved source of tier [{}]) has more distinct keys ({}) than MAX_KEYS ({}). Excess keys will be rejected."),
			Source, InHandle, SourceRegistry.Num(), goap::WorldState_MaxKeys);
	}

	// Resolve raw → typed ActionDef.
	InActions._ActionDefs.Reset();
	InActions._ActionDefs.Reserve(RawActions.Num());

	for (auto Index = int32{0}; Index < RawActions.Num(); ++Index)
	{
		const auto& Raw = RawActions[Index];
		auto Def = goap::FActionDef{};
		Def.ActionIndex = Index;
		Def.ActionClass = Raw.Class;
		Def.Cost        = Raw.Cost;
		Def.ActionTag   = Raw.ActionTag;

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

	// Resolve root-tier's _InitialGoal_RootOnly into typed _Current._Goal.
	// (Non-root tiers get their goal injected from parent action's Effects in
	// ChainUpdate.)
	if (NOT InParams.Get_InitialGoal_RootOnly().IsEmpty() && InCurrent._Goal.IsEmpty())
	{
		for (const auto& Cond : InParams.Get_InitialGoal_RootOnly())
		{
			const auto Key = SourceRegistry.Find(Cond.Get_Key());
			if (Key != goap::InvalidGoapKey)
			{
				InCurrent._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
			}
		}
	}
}

// ====================================================================================================================
// AUTO REPLAN
// ====================================================================================================================

auto
	FProcessor_Goap_Tier_AutoReplan::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Params& InParams,
		FFragment_Goap_Tier_ReplanThrottle& InThrottle) -> void
{
	// Don't replan until Setup completes.
	if (InHandle.Has<FTag_Goap_Tier_RequiresSetup>()) { return; }

	InThrottle._SecondsSinceLastReplan += InDeltaT.Get_Seconds();

	const auto IsInitialPlanPending = InHandle.Has<FTag_Goap_Tier_RequiresInitialPlan>();
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

	const auto ThrottleElapsed = InThrottle._SecondsSinceLastReplan
		>= InParams.Get_MinReplanIntervalSeconds();

	const auto ShouldFire = IsInitialPlanPending || (PolicyAllowsReplan && ThrottleElapsed);
	if (NOT ShouldFire) { return; }

	auto& Requests = InHandle.AddOrGet<FFragment_Goap_Tier_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Tier_Plan{});

	InThrottle._SecondsSinceLastReplan = 0.0f;
	InHandle.Try_Remove<FTag_Goap_Dirty_WorldState>();
	InHandle.Try_Remove<FTag_Goap_Dirty_Cost>();
	InHandle.Try_Remove<FTag_Goap_Tier_RequiresInitialPlan>();
}

// ====================================================================================================================
// HANDLE REQUESTS — variant dispatch
// ====================================================================================================================

auto
	FProcessor_Goap_Tier_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Tier_Current& InCurrent,
		FFragment_Goap_Tier_Actions& InActions,
		const FFragment_Goap_Tier_Requests& InRequests,
		FFragment_Goap_Tier_SearchState& InSearchState,
		FFragment_Goap_Tier_Result& InResult,
		FFragment_Goap_Tier_PlanContext& InPlanContext) const -> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Tier_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_Plan>)
			{
				// -- Plan request ---------------------------------------------
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();

				++InCurrent._PlanAttemptCount;

				const auto Source = InCurrent.Get_WorldStateSource_Resolved();
				if (NOT ck::IsValid(Source))
				{
					InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
					InCurrent._Plan.Reset();
					InCurrent._PlanCost = 0.0f;
					UUtils_Signal_OnGoap_Tier_PlanFailed::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
					return;
				}

				const auto& SourceWorldState = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_Values>().Get_Values();

				// Check if goal already satisfied — emit empty plan.
				const auto IsGoalSatisfied = [&]() -> bool
				{
					for (const auto& C : InCurrent._Goal)
					{
						if (NOT SourceWorldState.Satisfies(C)) { return false; }
					}
					return true;
				}();

				if (InCurrent._Goal.IsEmpty() || IsGoalSatisfied)
				{
					InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
					InCurrent._Plan.Reset();
					InCurrent._PlanCost = 0.0f;
					UUtils_Signal_OnGoap_Tier_PlanComplete::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
							TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
					return;
				}

				InCurrent._PlanStatus = ECk_GoapPlanStatus::Planning;
				InCurrent._Plan.Reset();
				InCurrent._PlanCost = 0.0f;

				// Build A* graph + seed search.
				const auto GoalConditions = BuildConstraintSet(InCurrent._Goal);
				auto Graph = goap::FGoapGraph{SourceWorldState, InActions._ActionDefs, GoalConditions};
				InPlanContext._Graph = Graph;

				constexpr auto GoalSentinel = TNumericLimits<int32>::Max();
				InSearchState._State = astar::TSearchState<int32, goap::FGoapGraph>{
					MoveTemp(Graph), 0, GoalSentinel};

				InResult._Path.Reset();
				InResult._TotalCost = 0.0f;
				InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
				InResult._TotalIterations = 0;
				InResult._TotalTimeMicroseconds = 0;

				InHandle.Add<FTag_AStar_SearchActive>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_CancelPlan>)
			{
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();
				InSearchState._State = {};
				InCurrent._PlanStatus = ECk_GoapPlanStatus::Idle;
				InCurrent._Plan.Reset();
				InCurrent._PlanCost = 0.0f;
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetGoal>)
			{
				// Resolve authored conditions via the tier's WS registry.
				InCurrent._Goal.Reset();
				InCurrent._InvalidGoal.Reset();

				const auto Source = InCurrent.Get_WorldStateSource_Resolved();
				if (NOT ck::IsValid(Source))
				{
					// Can't resolve — store as invalid for diagnostics.
					for (const auto& Cond : InTypedRequest.Get_Goal())
					{
						InCurrent._InvalidGoal.Add(Cond);
					}
					return;
				}

				const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
				for (const auto& Cond : InTypedRequest.Get_Goal())
				{
					const auto Key = Registry.Find(Cond.Get_Key());
					if (Key == goap::InvalidGoapKey)
					{
						InCurrent._InvalidGoal.Add(Cond);
					}
					else
					{
						InCurrent._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
					}
				}
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetActionCost>)
			{
				auto Changed = false;
				for (auto& Def : InActions._ActionDefs)
				{
					if (Def.ActionClass == InTypedRequest.Get_ActionClass())
					{
						if (Def.Cost != InTypedRequest.Get_Cost())
						{
							Def.Cost = InTypedRequest.Get_Cost();
							Changed = true;
						}
						break;
					}
				}
				if (Changed) { InHandle.AddOrGet<FTag_Goap_Dirty_Cost>(); }
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetReplanInterval>)
			{
				// _MinReplanIntervalSeconds lives in InParams (const here);
				// but the throttle reads from params each frame, so writing
				// here would be ignored. Surface mutation via the throttle
				// fragment instead — TODO: extend FFragment_Goap_Tier_ReplanThrottle
				// with a mutable interval if runtime tuning is needed.
				// For now: warn.
				ck::goap::Warning(
					TEXT("Request_SetReplanInterval not yet wired for per-tier runtime tuning — set in TierParams at AddTier."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetReplanPolicy>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanPolicy not yet wired for per-tier runtime tuning — set in TierParams at AddTier."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetSearchBudget>)
			{
				InAStarParams.Set_BudgetMicroseconds(InTypedRequest.Get_SearchBudgetMicroseconds());
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Tier_SetCostThreshold>)
			{
				InAStarParams.Set_CostThreshold(InTypedRequest.Get_CostThreshold());
			}
		}));
	});
}

// ====================================================================================================================
// HANDLE RESULT — A* completion → plan / fire signals
// ====================================================================================================================

auto
	FProcessor_Goap_Tier_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Tier_Result& InResult,
		const FFragment_Goap_Tier_PlanContext& InPlanContext,
		FFragment_Goap_Tier_Current& InCurrent) -> void
{
	InHandle.Remove<FTag_AStar_SearchComplete>();

	const auto& Graph = InPlanContext.Get_Graph();

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

			UUtils_Signal_OnGoap_Tier_PlanComplete::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
					MoveTemp(ActionClasses), InResult._TotalCost}));
			break;
		}

		case ECk_AStarSearchStatus::Failed:
		{
			InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Tier_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		case ECk_AStarSearchStatus::CostThresholdReached:
		{
			InCurrent._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Tier_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		default:
			break;
	}
}

// ====================================================================================================================

} // namespace ck
