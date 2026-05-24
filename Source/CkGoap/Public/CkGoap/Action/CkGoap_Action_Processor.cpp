#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/Action/CkGoap_Action_Utils.h"  // PR-B.1b Stage 3: CastChecked for Planner-as-Action
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // PR-B.1b Stage 0: resolve owning Planner for signal payload

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "Algo/Reverse.h"

// ====================================================================================================================

// PR-B.1b Stage 3: per-Action Setup stays Action-tier (per-Action CDO
// extraction). The remaining A*-pipeline processors are now Planner-tier.
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_AutoReplan);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_EndPlay);

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

	// PR-B.1b Stage 5 — A Planner's candidate operators are its own direct
	// children. No more implicit-root indirection; every AddAction adds a
	// direct child of the Planner. Promoted mid-tier Planners read children
	// from their own Tree fragment (the Planner-cast IS the Action that owns
	// the tree); top-level Planners read from their ActionCatalogIndex.
	auto Goap_Planner_GetCandidateChildren(const FCk_Handle_Goap_Planner& InPlanner)
		-> TArray<FCk_Handle_Goap_Action>
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }

		if (InPlanner.template Has<FFragment_Goap_Action_Tree>())
		{
			return InPlanner.template Get<FFragment_Goap_Action_Tree>().Get_ChildActions();
		}

		auto Result = TArray<FCk_Handle_Goap_Action>{};
		if (NOT InPlanner.template Has<FFragment_Goap_Planner_ActionCatalogIndex>()) { return Result; }
		const auto& Index = InPlanner.template Get<FFragment_Goap_Planner_ActionCatalogIndex>();
		Result.Reserve(Index.Get_TagToAction().Num());
		for (const auto& Pair : Index.Get_TagToAction())
		{
			if (ck::IsValid(Pair.Value)) { Result.Add(Pair.Value); }
		}
		return Result;
	}

	// PR-B.1b Stage 5 — resolve the "parent Planner" for parent-plan gating.
	// The intent: defer THIS Planner's Plan request if the Planner whose A*
	// search picks us as a candidate is still in-flight.
	//
	//   * Top-level Planner: no parent Planner — never gated. Return invalid.
	//   * Promoted mid-tier Planner: walk up the Action parent chain until we
	//     find a Planner entity. That's the Planner whose candidate set
	//     includes us. Returns invalid if none found.
	auto Goap_Planner_GetParentPlanner(const FCk_Handle_Goap_Planner& InPlanner)
		-> FCk_Handle_Goap_Planner
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }
		// Top-level Planner — no Action role, no parent.
		if (NOT InPlanner.template Has<FFragment_Goap_Action_Tree>()) { return {}; }

		const auto& Tree = InPlanner.template Get<FFragment_Goap_Action_Tree>();
		auto Walker = static_cast<FCk_Handle>(Tree.Get_ParentAction());
		constexpr auto MaxDepth = 64;
		for (auto Depth = 0; Depth < MaxDepth; ++Depth)
		{
			if (NOT ck::IsValid(Walker)) { break; }
			if (UCk_Utils_Goap_Planner_UE::Has(Walker))
			{
				return UCk_Utils_Goap_Planner_UE::CastChecked(Walker);
			}
			Walker = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Walker);
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
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Planner_WorldStateSource& InWSSource) -> void
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

	// PR-B.1b Stage 3: per-Action Setup no longer touches the Planner-side _Goal
	// fragment. The owning Planner's goal is resolved by
	// FProcessor_Goap_Planner_Setup (Planner-tier) which runs after every direct
	// child Action has completed its own Setup.
}

// ====================================================================================================================
// AUTO REPLAN
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_AutoReplan::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_ReplanThrottle& InThrottle) -> void
{
	// PR-B.1b Stage 3 — disable-toggle pipeline gate reads the Planner's own
	// FFragment_Goap_Planner_Current directly. Disabled Planners don't replan
	// (spec §3.3). Initial-plan / dirty tags remain set so re-enable resumes
	// from the deferred state.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	// Don't replan until the Planner-side Setup (cycle detection + goal
	// resolution) has completed. The tag remains set until
	// FProcessor_Goap_Planner_Setup removes it.
	if (InHandle.Has<FTag_Goap_Planner_RequiresSetup>()) { return; }

	// PR-B.1b Stage 3: defer if the Planner's WS isn't resolved yet. This
	// happens for promoted mid-tier Planners before their first activation —
	// the activation walk (DoActivatePlanner) sets _Resolved and re-issues
	// RequiresInitialPlan. Without this gate, a premature plan request would
	// reach HandleRequests with an invalid Source and fire spurious
	// PlanFailed broadcasts.
	if (NOT ck::IsValid(InWSSource.Get_Resolved())) { return; }

	InThrottle._SecondsSinceLastReplan += InDeltaT.Get_Seconds();

	const auto IsInitialPlanPending = InHandle.Has<FTag_Goap_Planner_RequiresInitialPlan>();
	const auto WSDirty   = InHandle.Has<FTag_Goap_Dirty_WorldState>();
	const auto CostDirty = InHandle.Has<FTag_Goap_Dirty_Cost>();

	// PR-B.1b Stage 5: replan-policy + interval live on PlannerParams.
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

	auto& Requests = InHandle.AddOrGet<FFragment_Goap_Planner_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Planner_Plan{});

	InThrottle._SecondsSinceLastReplan = 0.0f;
	InHandle.Try_Remove<FTag_Goap_Dirty_WorldState>();
	InHandle.Try_Remove<FTag_Goap_Dirty_Cost>();
	InHandle.Try_Remove<FTag_Goap_Planner_RequiresInitialPlan>();
}

// ====================================================================================================================
// HANDLE REQUESTS — variant dispatch
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Goal& InGoal,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		const FFragment_Goap_Planner_Requests& InRequests,
		FFragment_Goap_Planner_SearchState& InSearchState,
		FFragment_Goap_Planner_Result& InResult,
		FFragment_Goap_Planner_PlanContext& InPlanContext) const -> void
{
	(void)InParams;  // reserved (replan-policy reads moved to AutoReplan)
	// PR-B.1b Stage 3 — disable-toggle pipeline gate reads InCurrent directly.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	// Parent-plan gating: if THIS Planner has a parent whose plan is still in
	// flight, defer Plan requests by re-arming the initial-plan tag for next
	// frame. Top-level Planners (no parent) are never gated. The "parent" of
	// a Planner is resolved via Path-A bridge: promoted mid-tier Planners
	// carry FFragment_Goap_Action_Tree with _ParentAction. PR-B.1b Stage 5
	// will simplify this when implicit-root + dual-stamp goes away.
	const auto IsParentPlanInFlight = [&]() -> bool
	{
		const auto Parent = Goap_Planner_GetParentPlanner(InHandle);
		if (NOT ck::IsValid(Parent)) { return false; }

		if (Parent.template Has<FTag_Goap_Planner_PlanInFlight>()) { return true; }

		const auto& ParentPlanState = Parent.template Get<FFragment_Goap_Planner_PlanState>();
		switch (ParentPlanState.Get_PlanStatus())
		{
			case ECk_GoapPlanStatus::Planning:
				return true;
			case ECk_GoapPlanStatus::Idle:
				// Idle means parent hasn't planned YET. If the parent has the
				// initial-plan tag pending, we should defer; otherwise the parent
				// is intentionally not planning and we should not block.
				return Parent.template Has<FTag_Goap_Planner_RequiresInitialPlan>();
			default:
				return false;
		}
	}();

	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Planner_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_Plan>)
			{
				if (IsParentPlanInFlight)
				{
					// Parent is still planning (or has never planned yet) —
					// drop this Plan request and re-arm the initial-plan tag so
					// AutoReplan re-enqueues a Plan request on the NEXT frame.
					// We can't simply re-add the request to the queue:
					// MarkedDirtyBy = FFragment_Goap_Planner_Requests means
					// rewriting the fragment marks it dirty again, and the pump
					// will re-run HandleRequests this same frame, looping until
					// the pump limit fires. The tag-driven path takes effect
					// next frame, by which time the parent's plan may have
					// settled and released the gate.
					InHandle.AddOrGet<FTag_Goap_Planner_RequiresInitialPlan>();
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
					InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();
					UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
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
					InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();
					UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
							TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
					return;
				}

				InPlanState._PlanStatus = ECk_GoapPlanStatus::Planning;
				InPlanState._Plan.Reset();
				InPlanState._PlanCost = 0.0f;

				// Plan now in flight — gate any child Planners from starting their
				// own plans until our terminal status is reached. Removed in
				// HandleResult / CancelPlan / early-out paths above.
				InHandle.AddOrGet<FTag_Goap_Planner_PlanInFlight>();

				// PR-B.1b Stage 5: candidate operators are this Planner's own
				// direct children (no implicit-root indirection).
				//
				// IMPORTANT: each candidate's ActionIndex MUST be set to its
				// position in the Candidates array. The FGoapGraph stores this
				// index in EdgeActions; HandleResult uses it to map A* path
				// edges back to the candidate set. Leaving ActionIndex at its
				// default INDEX_NONE causes Cost() to return float::Max() and
				// Get_ActionForEdge to return INDEX_NONE, which silently drops
				// every edge from the produced plan (PlanFound + empty plan).
				const auto ChildHandles = Goap_Planner_GetCandidateChildren(InHandle);
				auto Candidates = TArray<goap::FActionDef>{};
				Candidates.Reserve(ChildHandles.Num());
				for (const auto& ChildHandle : ChildHandles)
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }
					const auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					auto Candidate = ChildDef.AsActionDef();
					Candidate.ActionIndex = Candidates.Num();
					Candidates.Add(MoveTemp(Candidate));
				}

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
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_CancelPlan>)
			{
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();
				InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();
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

				InHandle.AddOrGet<FTag_Goap_Planner_RequiresInitialPlan>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetActionCost>)
			{
				// Cost lives on a child Action's Action_Definition._Cost (and
				// its mirrored _CachedActionDef.Cost consumed by the A* search).
				// Look up the child by class via this Planner's candidate set
				// and mutate its def directly.
				const auto TargetClass = InTypedRequest.Get_ActionClass();
				const auto NewCost = InTypedRequest.Get_Cost();

				const auto Candidates = Goap_Planner_GetCandidateChildren(InHandle);
				for (auto ChildHandle : Candidates)
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }

					const auto& ChildParams = ChildHandle.template Get<FFragment_Goap_Action_Params>();
					if (ChildParams.Get_ActionClass() != TargetClass) { continue; }

					auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					ChildDef._Cost = NewCost;
					ChildDef._CachedActionDef.Cost = NewCost;
					break;
				}

				InHandle.AddOrGet<FTag_Goap_Dirty_Cost>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetReplanInterval>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanInterval not yet wired for runtime tuning — set in PlannerParams at Add time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetReplanPolicy>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanPolicy not yet wired for runtime tuning — set in PlannerParams at Add time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetSearchBudget>)
			{
				InAStarParams.Set_BudgetMicroseconds(InTypedRequest.Get_SearchBudgetMicroseconds());
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetCostThreshold>)
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
	FProcessor_Goap_Planner_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_Result& InResult,
		const FFragment_Goap_Planner_PlanContext& InPlanContext,
		FFragment_Goap_Planner_PlanState& InPlanState) -> void
{
	// PR-B.1b Stage 3 — disable-toggle pipeline gate reads InCurrent directly.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	InHandle.Remove<FTag_AStar_SearchComplete>();

	// Any terminal status releases the parent-plan gate for our children.
	// (No-op if the tag isn't present, e.g. an early-out path that already
	// removed it before broadcasting.)
	InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();

	switch (InResult._SearchStatus)
	{
		case ECk_AStarSearchStatus::Complete:
		{
			// Map A* path edges → child Action handles.
			//
			// The A* path is a sequence of state-pool node indices. For each
			// consecutive pair (Path[i], Path[i+1]), the FGoapGraph knows the
			// action index that bridged them; that ActionDef's ActionClass is
			// our match key into this Planner's candidate children.
			//
			// The search is regressive, so the A* path runs goal -> current.
			// To produce a forward execution plan we walk edges in order and
			// reverse at the end.
			const auto ChildHandles = Goap_Planner_GetCandidateChildren(InHandle);
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

			UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
					InPlanState.Get_PlanClasses(), InResult._TotalCost}));
			break;
		}

		case ECk_AStarSearchStatus::Failed:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		case ECk_AStarSearchStatus::CostThresholdReached:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;
			UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		default:
			break;
	}
}

// ====================================================================================================================

} // namespace ck
