#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "Algo/Reverse.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_AutoReplan);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_EndPlay);

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
// SETUP — per-Action CDO extraction
//
// In the unified model, each Action entity holds ONE def (its own). The
// Action's _ActionClass is the CDO source. Phase U1 keeps the old
// _ActionClasses fragment populated by legacy paths so AddAction_ToAction
// (U2) can fan in here; the per-Action def is the FIRST element of the
// resolved list. Effects' resolved form is staged into Action_Definition's
// fields so the planner can consume them as the Action's goal at activation.
// ====================================================================================================================

auto
	FProcessor_Goap_Action_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		const FFragment_Goap_Action_ActionClasses& InClasses,
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Action_Current& InCurrent) -> void
{
	// Setup must wait for the WS source to be resolved. Top-level Actions
	// resolve it at AddAction_ToActionSet time (if override is valid);
	// non-root Actions get it at activation time via ChainUpdate. If the
	// resolved source is invalid, defer to next frame WITHOUT removing the
	// setup tag.
	const auto Source = InCurrent.Get_WorldStateSource_Resolved();
	if (NOT ck::IsValid(Source))
	{
		return;
	}

	InHandle.Remove<FTag_Goap_Action_RequiresSetup>();

	auto& SourceRegistry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

	// Extract THIS Action's CDO into the Action_Definition shape.
	const auto ActionClass = InParams.Get_ActionClass();
	if (ck::IsValid(ActionClass))
	{
		if (auto* CDO = ActionClass.GetDefaultObject(); ck::IsValid(CDO))
		{
			CDO->DefineAction();

			InActionDef._Preconditions = CDO->_Preconditions;
			InActionDef._Effects       = CDO->_Effects;
			InActionDef._Cost          = CDO->_Cost;

			// Register Action's tags in the resolved WS registry.
			for (const auto& Pre : InActionDef._Preconditions) { SourceRegistry.FindOrRegister(Pre.Key); }
			for (const auto& Eff : InActionDef._Effects)       { SourceRegistry.FindOrRegister(Eff.Key); }
		}
	}

	// Register root-Action's initial-goal keys too (so the resolved goal hits
	// valid slots).
	for (const auto& Cond : InParams.Get_InitialGoal_RootOnly())
	{
		SourceRegistry.FindOrRegister(Cond.Get_Key());
	}

	if (SourceRegistry.Num() > goap::WorldState_MaxKeys)
	{
		ck::goap::Warning(
			TEXT("GOAP WorldState [{}] (resolved source of Action [{}]) has more distinct keys ({}) than MAX_KEYS ({}). Excess keys will be rejected."),
			Source, InHandle, SourceRegistry.Num(), goap::WorldState_MaxKeys);
	}

	// Build this Action's _CachedActionDef from its own preconditions / effects /
	// cost. The parent Action's planner consumes this as a candidate operator.
	{
		auto& Def = InActionDef._CachedActionDef;
		Def = goap::FActionDef{};
		Def.ActionClass = ActionClass;
		Def.Cost = InActionDef._Cost;

		// Action_Tag identity (debug only — chain matching uses entity handles).
		if (ck::IsValid(ActionClass))
		{
			Def.ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(ActionClass);
		}

		Def.Preconditions.Reserve(InActionDef._Preconditions.Num());
		for (const auto& Pre : InActionDef._Preconditions)
		{
			const auto Resolved = ResolveCondition(SourceRegistry, Pre);
			if (Resolved.IsValid()) { Def.Preconditions.Add(Resolved); }
		}

		Def.Effects.Reserve(InActionDef._Effects.Num());
		for (const auto& Eff : InActionDef._Effects)
		{
			const auto Resolved = ResolveEffect(SourceRegistry, Eff);
			if (Resolved.IsValid()) { Def.Effects.Add(Resolved); }
		}
	}

	// Pre-compute _GoalFromEffects (authored form). Child Actions copy this into
	// their _Current._Goal at activation. We store the authored shape here and
	// the consuming ChainUpdate resolves into runtime FWorldStateCondition via
	// the resolved WS registry of the activating Action.
	InActionDef._GoalFromEffects.Reset();
	InActionDef._GoalFromEffects.Reserve(InActionDef._Effects.Num());
	for (const auto& Eff : InActionDef._Effects)
	{
		InActionDef._GoalFromEffects.Add(FCk_GoapWS_Condition_Authored{Eff.Key, Eff.Value});
	}

	// Validate effect keys against this Action's resolved WS. Any effect whose
	// raw tag isn't in the registry goes into _InvalidGoal for diagnostic
	// surfacing via Get_InvalidGoal. Effects are static (CDO-extracted) so
	// this validation is one-shot at Setup time.
	//
	// Note: the loops above also call FindOrRegister on every effect key, so
	// in normal flow _InvalidGoal will be empty here. It only fills when the
	// registry has overflowed past WorldState_MaxKeys (warned about above) —
	// FindOrRegister silently rejects further registrations in that case and
	// Find returns InvalidGoapKey. This is the diagnostic the debugger surfaces.
	InActionDef._InvalidGoal.Reset();
	for (const auto& Eff : InActionDef._Effects)
	{
		if (SourceRegistry.Find(Eff.Key) == goap::InvalidGoapKey)
		{
			InActionDef._InvalidGoal.Add(FCk_GoapWS_Condition_Authored{Eff.Key, Eff.Value});
		}
	}

	if (InActionDef._InvalidGoal.Num() > 0)
	{
		ck::goap::Verbose(
			TEXT("Action [{}] has [{}] effect key(s) not in resolved WS registry."),
			InHandle, InActionDef._InvalidGoal.Num());
	}

	// _ActionClasses retained as a legacy collection (read by no live code in
	// the unified model). Candidate sets are built dynamically from
	// Action_Tree._ChildActions at plan-request time.
	(void)InClasses;

	// Resolve root-Action's _InitialGoal_RootOnly into typed _Current._Goal.
	// (Non-root Actions get their goal injected from parent action's Effects in
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
	FProcessor_Goap_Action_AutoReplan::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_Goap_Action_ReplanThrottle& InThrottle) -> void
{
	// Don't replan until Setup completes.
	if (InHandle.Has<FTag_Goap_Action_RequiresSetup>()) { return; }

	InThrottle._SecondsSinceLastReplan += InDeltaT.Get_Seconds();

	const auto IsInitialPlanPending = InHandle.Has<FTag_Goap_Action_RequiresInitialPlan>();
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

	auto& Requests = InHandle.AddOrGet<FFragment_Goap_Action_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Action_Plan{});

	InThrottle._SecondsSinceLastReplan = 0.0f;
	InHandle.Try_Remove<FTag_Goap_Dirty_WorldState>();
	InHandle.Try_Remove<FTag_Goap_Dirty_Cost>();
	InHandle.Try_Remove<FTag_Goap_Action_RequiresInitialPlan>();
}

// ====================================================================================================================
// HANDLE REQUESTS — variant dispatch
// ====================================================================================================================

auto
	FProcessor_Goap_Action_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Action_Current& InCurrent,
		FFragment_Goap_Action_Definition& InActionDef,
		const FFragment_Goap_Action_Requests& InRequests,
		FFragment_Goap_Action_SearchState& InSearchState,
		FFragment_Goap_Action_Result& InResult,
		FFragment_Goap_Action_PlanContext& InPlanContext) const -> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Action_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_Plan>)
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
					UUtils_Signal_OnGoap_Action_PlanFailed::Broadcast(
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
					UUtils_Signal_OnGoap_Action_PlanComplete::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
							TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
					return;
				}

				InCurrent._PlanStatus = ECk_GoapPlanStatus::Planning;
				InCurrent._Plan.Reset();
				InCurrent._PlanCost = 0.0f;

				// Build the planner's candidate operator set from this Action's
				// children. Each child Action carries its own pre-built
				// _CachedActionDef (resolved against this Action's WS at Setup).
				const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
				auto Candidates = TArray<goap::FActionDef>{};
				Candidates.Reserve(Tree.Get_ChildActions().Num());
				for (const auto& ChildHandle : Tree.Get_ChildActions())
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }
					const auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					Candidates.Add(ChildDef.AsActionDef());
				}
				(void)InActionDef;

				const auto GoalConditions = BuildConstraintSet(InCurrent._Goal);
				auto Graph = goap::FGoapGraph{SourceWorldState, Candidates, GoalConditions};
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
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_CancelPlan>)
			{
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();
				InSearchState._State = {};
				InCurrent._PlanStatus = ECk_GoapPlanStatus::Idle;
				InCurrent._Plan.Reset();
				InCurrent._PlanCost = 0.0f;
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetGoal>)
			{
				// Resolve authored conditions via the action's WS registry.
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
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetActionCost>)
			{
				// In the unified model, cost lives on a child Action's
				// Action_Definition._Cost (and its mirrored _CachedActionDef.Cost
				// consumed by the parent's planner). Look up the child by class
				// via Action_Tree and mutate its def directly.
				const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
				const auto TargetClass = InTypedRequest.Get_ActionClass();
				const auto NewCost = InTypedRequest.Get_Cost();

				for (auto ChildHandle : Tree.Get_ChildActions())
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }

					const auto& ChildParams = ChildHandle.template Get<FFragment_Goap_Action_Params>();
					if (ChildParams.Get_ActionClass() != TargetClass) { continue; }

					auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					ChildDef._Cost = NewCost;
					ChildDef._CachedActionDef.Cost = NewCost;
					break;
				}

				(void)InActionDef;
				InHandle.AddOrGet<FTag_Goap_Dirty_Cost>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetReplanInterval>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanInterval not yet wired for per-action runtime tuning — set in ActionParams at AddAction time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetReplanPolicy>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanPolicy not yet wired for per-action runtime tuning — set in ActionParams at AddAction time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetSearchBudget>)
			{
				InAStarParams.Set_BudgetMicroseconds(InTypedRequest.Get_SearchBudgetMicroseconds());
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_SetCostThreshold>)
			{
				InAStarParams.Set_CostThreshold(InTypedRequest.Get_CostThreshold());
			}
		}));
	});
}

// ====================================================================================================================
// HANDLE RESULT — A* completion → plan / fire signals
//
// In the unified model, _Plan is TArray<FCk_Handle_Goap_Action> — each entry is
// a child Action entity, found by mapping the A* path's edge ActionDef classes
// back to this Action's _ChildActions. The PlanComplete payload still carries
// the class array per the existing struct shape — we derive it via
// Get_PlanClasses().
// ====================================================================================================================

auto
	FProcessor_Goap_Action_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Result& InResult,
		const FFragment_Goap_Action_PlanContext& InPlanContext,
		FFragment_Goap_Action_Current& InCurrent) -> void
{
	InHandle.Remove<FTag_AStar_SearchComplete>();

	switch (InResult._SearchStatus)
	{
		case ECk_AStarSearchStatus::Complete:
		{
			// Map A* path edges → child Action handles.
			//
			// The A* path is a sequence of state-pool node indices. For each
			// consecutive pair (Path[i], Path[i+1]), the FGoapGraph knows the
			// action index that bridged them; that ActionDef's ActionClass is
			// our match key into this Action's _ChildActions.
			//
			// The search is regressive, so the A* path runs goal -> current.
			// To produce a forward execution plan we walk edges in order and
			// reverse at the end.
			const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
			const auto& ChildHandles = Tree.Get_ChildActions();
			const auto& Graph = InPlanContext.Get_Graph();
			const auto& Actions = Graph.Get_Actions();
			const auto& Path = InResult._Path;

			InCurrent._Plan.Reset();
			if (Path.Num() >= 2)
			{
				InCurrent._Plan.Reserve(Path.Num() - 1);
			}

			for (auto i = 0; i + 1 < Path.Num(); ++i)
			{
				const auto ActionIdx = Graph.Get_ActionForEdge(Path[i], Path[i + 1]);
				if (NOT Actions.IsValidIndex(ActionIdx)) { continue; }

				const auto& ActionDefOnEdge = Actions[ActionIdx];
				const auto TargetClass = ActionDefOnEdge.ActionClass;

				const auto* MatchingChild = ChildHandles.FindByPredicate(
					[&](const FCk_Handle_Goap_Action& InCandidate)
					{
						if (NOT ck::IsValid(InCandidate)) { return false; }
						const auto& CandidateParams = InCandidate.template Get<FFragment_Goap_Action_Params>();
						return CandidateParams.Get_ActionClass() == TargetClass;
					});

				CK_ENSURE_IF_NOT(MatchingChild != nullptr,
					TEXT("A* path contains ActionDef class [{}] not in parent Action [{}]'s child set."),
					TargetClass, InHandle)
				{ continue; }

				InCurrent._Plan.Add(*MatchingChild);
			}

			// Regressive search emits goal-to-current. Reverse for execution order.
			Algo::Reverse(InCurrent._Plan);

			InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
			InCurrent._PlanCost = InResult._TotalCost;

			UUtils_Signal_OnGoap_Action_PlanComplete::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
					InCurrent.Get_PlanClasses(), InResult._TotalCost}));
			break;
		}

		case ECk_AStarSearchStatus::Failed:
		{
			InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Action_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		case ECk_AStarSearchStatus::CostThresholdReached:
		{
			InCurrent._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Action_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		default:
			break;
	}
}

// ====================================================================================================================

} // namespace ck
