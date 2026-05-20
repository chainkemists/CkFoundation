#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

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

	// TODO(U3): Build _CachedActionDef from this Action's own (preconditions,
	// effects, cost) and populate _GoalFromEffects. Until U3 wires the
	// parent-as-planner flow, _CachedActionDef remains default.

	// TODO(U3): Build the planner candidate set from this Action's child
	// Actions' _CachedActionDef instead of a per-Action _ActionClasses list.
	// _ActionClasses is preserved as a legacy collection for now but is not
	// consumed below.
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

				// TODO(U3): Build the planner candidate set from this
				// Action's CHILDREN's _CachedActionDef (Action_Tree->Get_ChildActions).
				// Until then, planning with an empty candidate set produces a
				// degenerate Plan (effectively goal-already-satisfied behavior),
				// which is fine for U1 compile-clean.
				auto Candidates = TArray<goap::FActionDef>{};
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
				// TODO(U3): in the unified model, cost lives on a child
				// Action's Action_Definition._Cost. The right implementation
				// looks up the child Action by class via Action_Tree and
				// mutates its def directly. For now, no-op + dirty flag.
				(void)InTypedRequest;
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
// TODO(U3): Plan now stores TArray<FCk_Handle_Goap_Action>. The A* graph
// returns ActionIndex offsets which need to be mapped back to child Action
// entities via Action_Tree, not classes. For U1 compile-clean, the
// PlanComplete signal still uses the class array via Get_PlanClasses() but
// _Plan is emptied (no chain extension will actually happen until U3).
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

	(void)InPlanContext;

	switch (InResult._SearchStatus)
	{
		case ECk_AStarSearchStatus::Complete:
		{
			// TODO(U3): map Path edges → child Action entities via Action_Tree
			// and write them into _Plan. For now, plan is empty and the signal
			// payload reports an empty class array.
			InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = InResult._TotalCost;

			UUtils_Signal_OnGoap_Action_PlanComplete::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
					TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, InResult._TotalCost}));
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
