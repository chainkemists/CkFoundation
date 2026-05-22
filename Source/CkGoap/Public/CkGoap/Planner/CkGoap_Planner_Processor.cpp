#include "CkGoap/Planner/CkGoap_Planner_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // U11.2: snapshot active chain for OnActiveChainChanged payload
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_UpdateActivation);

// ====================================================================================================================

namespace ck_CkGoap_Planner_setup_internal
{
	// Iterative Tarjan SCC over a handle-keyed adjacency map. Returns SCCs as
	// TArray<TArray<HandleT>>. We deliberately avoid the textbook recursive
	// formulation: deep Action catalogs would consume the native call stack,
	// and recursion across UE's TMap/TSet semantics is awkward. The work-stack
	// form uses an explicit FFrame{Node, ChildIdx} record to remember where
	// each node's children-iteration left off.
	template<typename HandleT>
	auto TarjanScc(const TMap<HandleT, TArray<HandleT>>& InAdj)
		-> TArray<TArray<HandleT>>
	{
		auto Result   = TArray<TArray<HandleT>>{};
		auto Index    = TMap<HandleT, int32>{};
		auto Lowlink  = TMap<HandleT, int32>{};
		auto OnStack  = TSet<HandleT>{};
		auto Stack    = TArray<HandleT>{};
		auto NextIndex = int32{0};

		struct FFrame { HandleT Node; int32 ChildIdx; };
		auto WorkStack = TArray<FFrame>{};

		for (const auto& Pair : InAdj)
		{
			const auto& Root = Pair.Key;
			if (Index.Contains(Root)) { continue; }

			Index.Add(Root, NextIndex);
			Lowlink.Add(Root, NextIndex);
			++NextIndex;
			Stack.Push(Root);
			OnStack.Add(Root);
			WorkStack.Push({Root, 0});

			while (NOT WorkStack.IsEmpty())
			{
				auto& Frame = WorkStack.Top();
				const auto* ChildrenPtr = InAdj.Find(Frame.Node);

				if (ChildrenPtr != nullptr && Frame.ChildIdx < ChildrenPtr->Num())
				{
					const auto Child = (*ChildrenPtr)[Frame.ChildIdx];
					++Frame.ChildIdx;

					if (NOT Index.Contains(Child))
					{
						Index.Add(Child, NextIndex);
						Lowlink.Add(Child, NextIndex);
						++NextIndex;
						Stack.Push(Child);
						OnStack.Add(Child);
						WorkStack.Push({Child, 0});
					}
					else if (OnStack.Contains(Child))
					{
						Lowlink[Frame.Node] = FMath::Min(Lowlink[Frame.Node], Index[Child]);
					}
					continue;
				}

				// All children processed — finalise this node.
				const auto NodeHandle = Frame.Node;
				if (Lowlink[NodeHandle] == Index[NodeHandle])
				{
					auto Scc = TArray<HandleT>{};
					while (true)
					{
						const auto Popped = Stack.Pop();
						OnStack.Remove(Popped);
						Scc.Add(Popped);
						if (Popped == NodeHandle) { break; }
					}
					Result.Add(MoveTemp(Scc));
				}
				WorkStack.Pop();

				if (NOT WorkStack.IsEmpty())
				{
					auto& ParentFrame = WorkStack.Top();
					Lowlink[ParentFrame.Node] =
						FMath::Min(Lowlink[ParentFrame.Node], Lowlink[NodeHandle]);
				}
			}
		}
		return Result;
	}
}

namespace ck
{

// ====================================================================================================================
// SETUP — Cycle detection on the ActionSet's Action catalog.
//
// U5.2: iterative Tarjan SCC over the Action-tree _ChildActions edges. Any
// non-trivial SCC (size > 1, or size == 1 with a self-loop) is recorded in
// FFragment_Goap_Planner_Current._DependencyCycles as a diagnostic. The
// planner doesn't refuse to run on a cyclic catalog — diagnostics surface in
// the debugger, designer fixes it. Runs once after every catalog Action has
// completed its own Setup.
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_ActionCatalogIndex& InCatalogIndex) -> void
{
	const auto& Catalog = InCatalogIndex.Get_TagToAction();
	if (Catalog.IsEmpty())
	{
		InHandle.Remove<FTag_Goap_Planner_RequiresSetup>();
		return;
	}

	// Defer if any catalog Action hasn't completed Setup yet.
	for (const auto& Entry : Catalog)
	{
		const auto& Action = Entry.Value;
		if (Action.Has<FTag_Goap_Action_RequiresSetup>())
		{
			return;  // Defer; keep the ActionSet's RequiresSetup tag for retry.
		}
	}

	// Build the adjacency map keyed by Action handle. The directed edge goes
	// Parent -> Child (the planner walks down the chain), so a cycle means
	// a child's child... eventually contains the original parent.
	using ActionHandle = FCk_Handle_Goap_Action;
	auto Adj = TMap<ActionHandle, TArray<ActionHandle>>{};
	for (const auto& Entry : Catalog)
	{
		const auto& Action = Entry.Value;
		if (NOT ck::IsValid(Action)) { continue; }

		const auto& Tree = Action.template Get<FFragment_Goap_Action_Tree>();
		auto& Edges = Adj.Add(Action);
		Edges = Tree.Get_ChildActions();
	}

	const auto Sccs = ck_CkGoap_Planner_setup_internal::TarjanScc(Adj);

	InCurrent._DependencyCycles.Reset();
	for (const auto& Scc : Sccs)
	{
		// Filter out trivial SCCs — single node with no self-loop.
		if (Scc.Num() == 1)
		{
			const auto* Edges = Adj.Find(Scc[0]);
			const auto HasSelfLoop = (Edges != nullptr) && Edges->Contains(Scc[0]);
			if (NOT HasSelfLoop) { continue; }
		}

		auto ActionsInCycle = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};
		ActionsInCycle.Reserve(Scc.Num());

		// Compute precondition∩effect overlap across cycle Actions — these are
		// the WS keys that close the loop (something's effect satisfies the
		// next Action's precondition, all the way back round).
		auto PreconditionTags = TSet<FGameplayTag>{};
		auto EffectTags = TSet<FGameplayTag>{};

		for (const auto& ActionHandleInScc : Scc)
		{
			if (NOT ck::IsValid(ActionHandleInScc)) { continue; }

			const auto& Params = ActionHandleInScc.template Get<FFragment_Goap_Action_Params>();
			ActionsInCycle.Add(Params.Get_ActionClass());

			const auto& Def = ActionHandleInScc.template Get<FFragment_Goap_Action_Definition>();
			for (const auto& Pre : Def.Get_Preconditions()) { PreconditionTags.Add(Pre.Key); }
			for (const auto& Eff : Def.Get_Effects())       { EffectTags.Add(Eff.Key); }
		}

		auto CycleConditions = TArray<FGameplayTag>{};
		for (const auto& Tag : PreconditionTags)
		{
			if (EffectTags.Contains(Tag)) { CycleConditions.Add(Tag); }
		}

		InCurrent._DependencyCycles.Add(
			FCk_GoapDiagnostic_DependencyCycle{MoveTemp(ActionsInCycle), MoveTemp(CycleConditions)});
	}

	if (InCurrent._DependencyCycles.Num() > 0)
	{
		ck::goap::Warning(
			TEXT("ActionSet [{}] has [{}] dependency cycle(s) in its Action catalog."),
			InHandle, InCurrent._DependencyCycles.Num());
	}

	InHandle.Remove<FTag_Goap_Planner_RequiresSetup>();
}

// ====================================================================================================================
// UPDATE ACTIVATION — per-Planner activation transitions.
//
// Per spec §4.2: each Planner caches its previous Plan[0] in
// FFragment_Goap_Planner_Activation; on each tick, compares with the current
// Plan[0] and dispatches activate/deactivate transitions for composite sub-
// Planners. The "active chain" is no longer stored anywhere — Get_ActiveChain
// derives it from a Plan[0] walk starting at the top-level Planner's root
// Action.
//
// In the U11.2 transitional model, sub-Planners ARE Action entities — every
// composite Action carries FFragment_Goap_Planner_PlanState and runs its own
// A* planner. The processor walks Action entities (those with PlanState +
// Activation), not the top-level Planner entity (which doesn't run A*).
// ====================================================================================================================

namespace
{
	// "Is a Planner" in U11.2 = has children. Atomic Actions terminate the
	// activation walk and never get the per-sub-Planner activate/deactivate
	// treatment.
	auto Is_Composite(const FCk_Handle_Goap_Action& InAction) -> bool
	{
		if (NOT ck::IsValid(InAction)) { return false; }
		const auto& Tree = InAction.template Get<FFragment_Goap_Action_Tree>();
		return NOT Tree.Get_ChildActions().IsEmpty();
	}
}

// U11.2: re-resolve a sub-Planner's goal from its OWN _GoalAuthored (the
// authored, tag-keyed source-of-truth set at construction via PlannerParams /
// SetRootAction or at runtime via Request_SetGoal).
//
// Why we re-resolve here even though Setup already resolved once: the sub-
// Planner may carry a parent-inherited WS source whose registry differs from
// whatever was current at Setup time. Re-resolving against the activation-time
// WS registry guarantees keys resolve correctly under any WS-source override
// chain.
//
// Precondition: InPlanner's FFragment_Goap_Planner_WorldStateSource._Resolved
// is valid.
auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InPlanner) -> void
{
	auto& Goal = InPlanner.template Get<FFragment_Goap_Planner_Goal>();
	const auto& WSSource = InPlanner.template Get<FFragment_Goap_Planner_WorldStateSource>();

	Goal._Goal.Reset();
	Goal._InvalidGoal.Reset();

	// Action's effect-key validation (populated at Setup) is still surfaced as
	// a diagnostic — those reflect this Action's role as a *candidate operator*
	// for the parent's planner, independent of its own goal.
	const auto& Def = InPlanner.template Get<FFragment_Goap_Action_Definition>();
	Goal._InvalidGoal = Def.Get_InvalidGoal();

	const auto& Authored = Goal.Get_GoalAuthored();
	if (Authored.IsEmpty())
	{
		// No goal authored — planner emits empty plan / PlanFound immediately.
		return;
	}

	const auto& WS = WSSource.Get_Resolved();
	if (NOT ck::IsValid(WS))
	{
		// Can't resolve keys without a WS — record everything as invalid.
		for (const auto& Cond : Authored)
		{
			Goal._InvalidGoal.Add(Cond);
		}
		return;
	}

	auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(WS)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

	Goal._Goal.Reserve(Authored.Num());
	for (const auto& Cond : Authored)
	{
		const auto Key = Registry.FindOrRegister(Cond.Get_Key());
		if (Key == goap::InvalidGoapKey)
		{
			Goal._InvalidGoal.Add(Cond);
			continue;
		}
		Goal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent) -> void
{
	auto& ChildWSSource = InChild.template Get<FFragment_Goap_Planner_WorldStateSource>();
	const auto& ChildParams = InChild.template Get<FFragment_Goap_Action_Params>();

	// 1. Child's own override (if set) wins.
	const auto Override = ChildParams.Get_WorldStateSource_Override();
	if (ck::IsValid(Override))
	{
		ChildWSSource._Resolved = Override;
		return;
	}

	// 2. Inherit from the parent's resolved WS.
	if (ck::IsValid(InParent))
	{
		const auto& ParentWSSource = InParent.template Get<FFragment_Goap_Planner_WorldStateSource>();
		if (ck::IsValid(ParentWSSource.Get_Resolved()))
		{
			ChildWSSource._Resolved = ParentWSSource.Get_Resolved();
			return;
		}
	}

	// 3. Fall back to the top-level Planner's default WS source (lifetime owner
	// of every Action is the top-level Planner entity — see AddAction_ToAction
	// in CkGoap_Action_Utils.cpp).
	auto OwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InChild);
	if (OwnerEntity.template Has<FFragment_Goap_Planner_WorldStateSource>())
	{
		const auto& SetWS = OwnerEntity.template Get<FFragment_Goap_Planner_WorldStateSource>();
		ChildWSSource._Resolved = SetWS.Get_WorldStateSource();
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	auto& WSSource = InAction.template Get<FFragment_Goap_Planner_WorldStateSource>();
	auto WS = WSSource._Resolved;
	if (NOT ck::IsValid(WS)) { return; }

	auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.AddUnique(FCk_Handle{InAction});
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	auto& WSSource = InAction.template Get<FFragment_Goap_Planner_WorldStateSource>();
	auto WS = WSSource._Resolved;
	if (NOT ck::IsValid(WS)) { return; }

	auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.RemoveSwap(FCk_Handle{InAction});
}

// ====================================================================================================================
// ACTIVATE — turn a child sub-Planner on. Resolves WS, injects goal, subscribes
// to WS, sets RequiresInitialPlan, broadcasts OnPlannerActivated, flips
// _IsActive=true. Caller is responsible for the parent-Action's _ActiveParent
// bookkeeping (the FFragment_Goap_Action_Current field tracking which parent
// activated this child).
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoActivatePlanner(
		FCk_Handle_Goap_Action InPlanner,
		FCk_Handle_Goap_Action InParent) -> void
{
	if (NOT ck::IsValid(InPlanner)) { return; }

	auto& Activation = InPlanner.template Get<FFragment_Goap_Planner_Activation>();
	if (Activation._IsActive)
	{
		// Already active — no-op.
		return;
	}

	// Mark "ActiveParent" so the rest of the framework can introspect which
	// parent activated us.
	if (ck::IsValid(InParent))
	{
		auto& ChildCurrent = InPlanner.template Get<FFragment_Goap_Action_Current>();
		const auto& ParentParams = InParent.template Get<FFragment_Goap_Action_Params>();
		ChildCurrent._ActiveParentAction = ParentParams.Get_ActionClass();
	}

	// Resolve WS first so DoInjectGoalSynchronous can use the registry.
	DoResolveAndAssignWorldStateSource(InPlanner, InParent);
	DoInjectGoalSynchronous(InPlanner);
	DoSubscribeActionToWorldState(InPlanner);

	InPlanner.template AddOrGet<FTag_Goap_Action_RequiresInitialPlan>();

	Activation._IsActive = true;

	UUtils_Signal_OnGoap_Planner_Activated::Broadcast(
		InPlanner, ck::MakePayload(InPlanner, FCk_Goap_Payload_OnPlannerActivated{}));
}

// ====================================================================================================================
// DEACTIVATE — turn a sub-Planner off. Recursively deactivates any active
// descendants so their _IsActive flags don't go stale (otherwise re-activation
// of the same sub-tree would silently no-op the descendant chain).
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoDeactivatePlanner(FCk_Handle_Goap_Action InPlanner) -> void
{
	if (NOT ck::IsValid(InPlanner)) { return; }

	auto& Activation = InPlanner.template Get<FFragment_Goap_Planner_Activation>();
	if (NOT Activation._IsActive)
	{
		// Not active — no-op (defensive; we still clear any cached Plan0).
		Activation._LastActivatedPlan0 = {};
		return;
	}

	// Recurse into _LastActivatedPlan0 first so deeper layers settle before us.
	// We can't read this Planner's PlanState._Plan[0] because that may have
	// already mutated (the change is what triggered our deactivation). Use the
	// cached last-activated-plan0 instead — it represents the descendant
	// activation tree at the previous tick.
	{
		auto LastDescendant = Activation._LastActivatedPlan0;
		if (ck::IsValid(LastDescendant) && Is_Composite(LastDescendant))
		{
			DoDeactivatePlanner(LastDescendant);
		}
	}

	// Unsubscribe synchronously (avoids a deferred request that would land
	// after the next activation pass on this same Action).
	DoUnsubscribeActionFromWorldState(InPlanner);

	auto& Current   = InPlanner.template Get<FFragment_Goap_Action_Current>();
	auto& Goal      = InPlanner.template Get<FFragment_Goap_Planner_Goal>();
	auto& PlanState = InPlanner.template Get<FFragment_Goap_Planner_PlanState>();
	auto& WSSource  = InPlanner.template Get<FFragment_Goap_Planner_WorldStateSource>();

	Goal._Goal.Reset();
	Goal._InvalidGoal.Reset();
	Current._ActiveParentAction = nullptr;
	WSSource._Resolved = {};
	PlanState._Plan.Reset();
	PlanState._PlanStatus = ECk_GoapPlanStatus::Idle;

	// Release any pending plan-initial / in-flight gating tags — Planner is
	// leaving the chain and won't broadcast a terminal status to drop them itself.
	InPlanner.template Try_Remove<FTag_Goap_Action_RequiresInitialPlan>();
	InPlanner.template Try_Remove<FTag_Goap_Action_PlanInFlight>();

	Activation._IsActive = false;
	Activation._LastActivatedPlan0 = {};

	UUtils_Signal_OnGoap_Planner_Deactivated::Broadcast(
		InPlanner, ck::MakePayload(InPlanner, FCk_Goap_Payload_OnPlannerDeactivated{}));
}

// ====================================================================================================================
// ForEachEntity — per-Planner activation transition rule (spec §4.2).
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_UpdateActivation::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		const FFragment_Goap_Action_Tree& InTree,
		const FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Activation& InActivation) const -> void
{
	(void)InParams;
	(void)InTree;

	// Gate on this Planner being active. Top-level Planner's root Action is
	// activated at SetRootAction time; mid-tier sub-Planners are activated by
	// their parent's UpdateActivation pass.
	if (NOT InActivation._IsActive) { return; }

	// Also gate on the owning top-level Planner's enable toggle. The toggle
	// lives on FFragment_Goap_Planner_Current of the lifetime owner.
	{
		auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
		if (Owner.template Has<FFragment_Goap_Planner_Current>())
		{
			const auto& OwnerCurrent = Owner.template Get<FFragment_Goap_Planner_Current>();
			if (OwnerCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }
		}
	}

	if (InPlanState.Get_PlanStatus() != ECk_GoapPlanStatus::PlanFound &&
		InPlanState.Get_PlanStatus() != ECk_GoapPlanStatus::PlanFailed)
	{
		// Mid-decision — don't churn activation.
		return;
	}

	const auto OldStep0 = InActivation._LastActivatedPlan0;
	const auto NewStep0 = InPlanState.Get_Plan().IsEmpty()
		? FCk_Handle_Goap_Action{}
		: InPlanState.Get_Plan()[0];

	if (OldStep0 == NewStep0)
	{
		// No change — nothing to do.
		return;
	}

	// Snapshot the active chain (from the top-level Planner) BEFORE mutating
	// activation state, so OnGoap_Planner_ActiveChainChanged can carry a
	// pre-mutation _OldChain payload.
	auto TopLevelPlanner = FCk_Handle_Goap_Planner{};
	auto OldChainSnapshot = TArray<FCk_Handle_Goap_Action>{};
	{
		auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
		if (UCk_Utils_Goap_Planner_UE::Has(Owner))
		{
			TopLevelPlanner = UCk_Utils_Goap_Planner_UE::CastChecked(Owner);
			OldChainSnapshot = UCk_Utils_Goap_Planner_UE::Get_ActiveChain(TopLevelPlanner);
		}
	}

	// Deactivate the old Step0 if it changed AND it was a composite sub-Planner.
	if (ck::IsValid(OldStep0) && Is_Composite(OldStep0))
	{
		DoDeactivatePlanner(OldStep0);
	}

	// Activate the new Step0 if it's a composite sub-Planner.
	if (ck::IsValid(NewStep0) && Is_Composite(NewStep0))
	{
		auto& ChildActivation = NewStep0.template Get<FFragment_Goap_Planner_Activation>();
		if (NOT ChildActivation._IsActive)
		{
			DoActivatePlanner(NewStep0, InHandle);
		}
	}

	InActivation._LastActivatedPlan0 = NewStep0;

	// Broadcast OnGoap_Planner_ActiveChainChanged from the top-level Planner.
	// U11.2: the signal still fires per-frame any tier's activation changes —
	// the active chain (derived top-down) has mutated. Payload carries the
	// pre-mutation chain snapshot; consumers query the new chain via
	// Get_ActiveChain in the handler.
	if (ck::IsValid(TopLevelPlanner))
	{
		UUtils_Signal_OnGoap_Planner_ActiveChainChanged::Broadcast(
			TopLevelPlanner, ck::MakePayload(TopLevelPlanner,
				FCk_Goap_Payload_OnActiveChainChanged{OldChainSnapshot}));
	}
}

// ====================================================================================================================

} // namespace ck
