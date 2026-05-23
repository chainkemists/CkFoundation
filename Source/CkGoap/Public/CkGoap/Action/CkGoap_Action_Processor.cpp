#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // PR-B.1b Stage 0: resolve owning Planner for signal payload

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
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

	// PR-B.1b Stage 0 — given the Action entity that broadcasts a per-Planner
	// signal, resolve the owning Planner handle to put in the payload.
	//
	// Under Path A:
	//   * If the broadcasting Action carries the Planner role (promoted mid-tier
	//     composite), the entity IS the Planner — cast it.
	//   * Otherwise the broadcaster is the implicit-root Action of a top-level
	//     Planner — the Planner entity is the Action's lifetime owner.
	//
	// Returns an invalid Planner handle if neither resolution succeeds (e.g. the
	// Action is not under a Planner — should not happen in well-formed graphs).
	auto Goap_PRB1b_ResolveOwningPlanner(const FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Planner
	{
		if (NOT ck::IsValid(InAction)) { return {}; }

		if (UCk_Utils_Goap_Planner_UE::Has(InAction))
		{
			return UCk_Utils_Goap_Planner_UE::CastChecked(InAction);
		}

		auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAction);
		if (UCk_Utils_Goap_Planner_UE::Has(Owner))
		{
			return UCk_Utils_Goap_Planner_UE::CastChecked(Owner);
		}

		return {};
	}
}

// ====================================================================================================================
// SETUP — per-Action CDO extraction
//
// In the unified model, each Action entity holds ONE def (its own). The
// Action's _ActionClass is the CDO source. Phase U1 keeps the old
// _ActionClasses fragment populated by legacy paths so the unified AddAction
// path can fan in here; the per-Action def is the FIRST element of the
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
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_Goal& InGoal) -> void
{
	// Setup must wait for the WS source to be resolved. Top-level Actions
	// resolve it at AddAction time (if override is valid);
	// non-root Actions get it at activation time via ChainUpdate. If the
	// resolved source is invalid, defer to next frame WITHOUT removing the
	// setup tag.
	const auto Source = InWSSource.Get_Resolved();
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

	// U11.1: deliberately do NOT auto-register keys from _GoalAuthored. Goal
	// keys that don't appear in any Action's preconditions or effects are a
	// diagnostic condition (Get_InvalidGoal surfaces them) — registering them
	// here would mask the diagnostic and let the planner search toward a key
	// it can never affect. Setup just resolves the authored goal via Find,
	// dropping any keys that aren't already registered.

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

	// U11.1: resolve the Planner's authored goal into the typed _Goal slot.
	// Every Planner has its own _GoalAuthored (independent from any Action role
	// effects this entity may also carry). If empty, _Goal stays empty —
	// HandleRequests will short-circuit Plan requests to PlanFound + empty plan.
	// Setup only resolves keys already registered (CDO pre/effects) — keys
	// that don't resolve are silently dropped. _InvalidGoal is populated by
	// the Request_SetGoal handler (the canonical diagnostic surface), not by
	// Setup, to avoid double-counting when both construct-time goal and
	// runtime SetGoal touch the same entry.
	if (NOT InGoal._GoalAuthored.IsEmpty() && InGoal._Goal.IsEmpty())
	{
		for (const auto& Cond : InGoal._GoalAuthored)
		{
			const auto Key = SourceRegistry.Find(Cond.Get_Key());
			if (Key != goap::InvalidGoapKey)
			{
				InGoal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
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
		FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Goal& InGoal,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Action_Definition& InActionDef,
		const FFragment_Goap_Action_Requests& InRequests,
		FFragment_Goap_Action_SearchState& InSearchState,
		FFragment_Goap_Action_Result& InResult,
		FFragment_Goap_Action_PlanContext& InPlanContext) const -> void
{
	// Parent-plan gating: if THIS Action has a parent whose plan is still in
	// flight (or has never produced a terminal result yet), defer Plan requests
	// from this Action by re-enqueuing them. Other request types (SetGoal,
	// SetActionCost, cancel, etc.) drain normally — they are configuration
	// writes that don't depend on parent plan ordering.
	//
	// "In flight" = parent has FTag_Goap_Action_PlanInFlight, or parent's
	// PlanStatus is Idle/Planning (i.e. has never reached a terminal state).
	// Once parent reaches PlanFound / PlanFailed / CostThresholdReached, the
	// gate releases. Grandchildren defer naturally: while Root gates Mid,
	// Mid stays Idle, so Mid gates the grandchild via the same status check.
	const auto IsParentPlanInFlight = [&]() -> bool
	{
		const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
		const auto& Parent = Tree.Get_ParentAction();
		if (NOT ck::IsValid(Parent)) { return false; }

		if (Parent.template Has<FTag_Goap_Action_PlanInFlight>()) { return true; }

		const auto& ParentPlanState = Parent.template Get<FFragment_Goap_Planner_PlanState>();
		switch (ParentPlanState.Get_PlanStatus())
		{
			case ECk_GoapPlanStatus::Idle:
			case ECk_GoapPlanStatus::Planning:
				return true;
			default:
				return false;
		}
	}();

	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Action_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Action_Plan>)
			{
				if (IsParentPlanInFlight)
				{
					// Parent is still planning (or has never planned yet) —
					// drop this Plan request and re-arm the initial-plan tag so
					// AutoReplan re-enqueues a Plan request on the NEXT frame.
					// We can't simply re-add the request to FFragment_Goap_Action_Requests:
					// MarkedDirtyBy = FFragment_Goap_Action_Requests means
					// rewriting the fragment marks it dirty again, and the pump
					// will re-run HandleRequests this same frame, looping until
					// the pump limit fires. The tag-driven path takes effect
					// next frame, by which time the parent's plan may have
					// settled and released the gate.
					InHandle.AddOrGet<FTag_Goap_Action_RequiresInitialPlan>();
					return;
				}

				// -- Plan request ---------------------------------------------
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();

				++InPlanState._PlanAttemptCount;

				const auto Source = InWSSource.Get_Resolved();
				if (NOT ck::IsValid(Source))
				{
					InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
					InPlanState._Plan.Reset();
					InPlanState._PlanCost = 0.0f;
					InHandle.Try_Remove<FTag_Goap_Action_PlanInFlight>();
					{
						const auto OwningPlanner = Goap_PRB1b_ResolveOwningPlanner(InHandle);
						UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
							InHandle, ck::MakePayload(OwningPlanner, FCk_Goap_Payload_OnPlanFailed{}));
					}
					return;
				}

				// Flatten the override stack onto a local snapshot WS: base
				// values first, then each layer bottom-to-top (top wins). The
				// A* inner loop reads this snapshot exclusively — stack walks
				// stay out of the hot path. No-op cost when no overrides are
				// active (the common case).
				auto SourceWorldState = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_Values>().Get_Values();

				if (const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Has<FFragment_Goap_WorldState_OverrideStack>())
				{
					const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
						.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
					const auto& Stack = const_cast<FCk_Handle_Goap_WorldState&>(Source)
						.template Get<FFragment_Goap_WorldState_OverrideStack>();

					for (const auto& Layer : Stack.Get_Layers())
					{
						for (const auto& Kv : Layer.Values)
						{
							const auto FlatKey = Registry.Find(Kv.Key);
							if (FlatKey == goap::InvalidGoapKey) { continue; }
							SourceWorldState.Set(FlatKey, Kv.Value);
						}
					}
				}

				// Check if goal already satisfied — emit empty plan.
				const auto IsGoalSatisfied = [&]() -> bool
				{
					for (const auto& C : InGoal._Goal)
					{
						if (NOT SourceWorldState.Satisfies(C)) { return false; }
					}
					return true;
				}();

				if (InGoal._Goal.IsEmpty() || IsGoalSatisfied)
				{
					InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFound;
					InPlanState._Plan.Reset();
					InPlanState._PlanCost = 0.0f;
					InHandle.Try_Remove<FTag_Goap_Action_PlanInFlight>();
					{
						const auto OwningPlanner = Goap_PRB1b_ResolveOwningPlanner(InHandle);
						UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
							InHandle, ck::MakePayload(OwningPlanner, FCk_Goap_Payload_OnPlanComplete{
								TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
					}
					return;
				}

				InPlanState._PlanStatus = ECk_GoapPlanStatus::Planning;
				InPlanState._Plan.Reset();
				InPlanState._PlanCost = 0.0f;

				// Plan now in flight — gate any child Actions from starting their
				// own plans until our terminal status is reached. Removed in
				// HandleResult / CancelPlan / early-out paths above.
				InHandle.AddOrGet<FTag_Goap_Action_PlanInFlight>();

				// Build the planner's candidate operator set from this Action's
				// children. Each child Action carries its own pre-built
				// _CachedActionDef (resolved against this Action's WS at Setup).
				//
				// IMPORTANT: each candidate's ActionIndex MUST be set to its
				// position in the Candidates array. The FGoapGraph stores this
				// index in EdgeActions; HandleResult uses it to map A* path
				// edges back to the candidate set. Leaving ActionIndex at its
				// default INDEX_NONE causes Cost() to return float::Max() and
				// Get_ActionForEdge to return INDEX_NONE, which silently drops
				// every edge from the produced plan (PlanFound + empty plan).
				const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
				auto Candidates = TArray<goap::FActionDef>{};
				Candidates.Reserve(Tree.Get_ChildActions().Num());
				for (const auto& ChildHandle : Tree.Get_ChildActions())
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }
					const auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					auto Candidate = ChildDef.AsActionDef();
					Candidate.ActionIndex = Candidates.Num();
					Candidates.Add(MoveTemp(Candidate));
				}
				(void)InActionDef;

				const auto GoalConditions = BuildConstraintSet(InGoal._Goal);
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
				InHandle.Try_Remove<FTag_Goap_Action_PlanInFlight>();
				InSearchState._State = {};
				InPlanState._PlanStatus = ECk_GoapPlanStatus::Idle;
				InPlanState._Plan.Reset();
				InPlanState._PlanCost = 0.0f;
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetGoal>)
			{
				// Store authored goal and re-resolve via the action's WS registry.
				const auto NewAuthored = InTypedRequest.Get_NewGoal();

				InGoal._GoalAuthored = NewAuthored;
				InGoal._Goal.Reset();
				InGoal._InvalidGoal.Reset();

				const auto Source = InWSSource.Get_Resolved();
				if (NOT ck::IsValid(Source))
				{
					// Can't resolve — store as invalid for diagnostics.
					for (const auto& Cond : NewAuthored)
					{
						InGoal._InvalidGoal.Add(Cond);
					}
					return;
				}

				// Use Find (read-only) — goal keys that aren't in the WS registry
				// are diagnostics (Get_InvalidGoal) rather than silent additions.
				const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
				for (const auto& Cond : NewAuthored)
				{
					const auto Key = Registry.Find(Cond.Get_Key());
					if (Key == goap::InvalidGoapKey)
					{
						InGoal._InvalidGoal.Add(Cond);
					}
					else
					{
						InGoal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
					}
				}

				InHandle.AddOrGet<FTag_Goap_Action_RequiresInitialPlan>();
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
		FFragment_Goap_Planner_PlanState& InPlanState) -> void
{
	InHandle.Remove<FTag_AStar_SearchComplete>();

	// Any terminal status releases the parent-plan gate for our children.
	// (No-op if the tag isn't present, e.g. an early-out path that already
	// removed it before broadcasting.)
	InHandle.Try_Remove<FTag_Goap_Action_PlanInFlight>();

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

			InPlanState._Plan.Reset();
			if (Path.Num() >= 2)
			{
				InPlanState._Plan.Reserve(Path.Num() - 1);
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

				InPlanState._Plan.Add(*MatchingChild);
			}

			// Regressive search emits goal-to-current. Reverse for execution order.
			Algo::Reverse(InPlanState._Plan);

			InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFound;
			InPlanState._PlanCost = InResult._TotalCost;

			{
				const auto OwningPlanner = Goap_PRB1b_ResolveOwningPlanner(InHandle);
				UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
					InHandle, ck::MakePayload(OwningPlanner, FCk_Goap_Payload_OnPlanComplete{
						InPlanState.Get_PlanClasses(), InResult._TotalCost}));
			}
			break;
		}

		case ECk_AStarSearchStatus::Failed:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;
			{
				const auto OwningPlanner = Goap_PRB1b_ResolveOwningPlanner(InHandle);
				UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
					InHandle, ck::MakePayload(OwningPlanner, FCk_Goap_Payload_OnPlanFailed{}));
			}
			break;
		}

		case ECk_AStarSearchStatus::CostThresholdReached:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;
			{
				const auto OwningPlanner = Goap_PRB1b_ResolveOwningPlanner(InHandle);
				UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
					InHandle, ck::MakePayload(OwningPlanner, FCk_Goap_Payload_OnPlanFailed{}));
			}
			break;
		}

		default:
			break;
	}
}

// ====================================================================================================================

} // namespace ck
