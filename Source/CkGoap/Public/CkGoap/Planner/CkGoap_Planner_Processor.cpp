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
// SETUP — Per-Planner cycle detection over this Planner's direct children.
//
// U11.4 — Spec §7.2: each Planner runs Tarjan SCC over its own direct children's
// PRECONDITION/EFFECT dependency graph. "Direct children" of a Planner are its
// candidate operators — the Actions it considers when planning. In the unified
// Action-as-Planner model:
//   - A Planner that is itself an Action (promoted via PromoteActionToPlanner)
//     has `FFragment_Goap_Action_Tree`; its direct children are its own
//     `_ChildActions`.
//   - A top-level Planner (created via Add) is not an Action; its direct
//     children are the root Action's `_ChildActions` (the root Action is what
//     subdivides the plan on behalf of the top-level Planner in the current
//     transitional model).
//
// The edge model is the precondition/effect dependency graph: for sibling
// Actions A and B, if any effect (Key,Value) in A's `_Effects` matches any
// precondition (Key,Value) in B's `_Preconditions`, add edge A -> B (read:
// "B depends on A"). This is the real planner-relevant dependency graph —
// the tree-edge model used previously was a no-op since a tree has no cycles
// by construction.
//
// A cycle (SCC of size > 1, or a self-loop) means the candidate operators at
// this tier mutually require each other's effects to satisfy each other's
// preconditions — the planner cannot satisfy any one of them without first
// running another that itself transitively depends on the first.
// Non-trivial SCCs are recorded in `FFragment_Goap_Planner_Current.
// _DependencyCycles` as a diagnostic. The planner doesn't refuse cyclic
// catalogs — designers fix them via the debugger surface.
//
// Defers if any direct child still has `FTag_Goap_Action_RequiresSetup` so the
// per-Action `_Definition` (used to compute precondition/effect overlap below)
// has been populated.
// ====================================================================================================================

auto
	FProcessor_Goap_Planner_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_ActionCatalogIndex& InCatalogIndex) -> void
{
	(void)InCatalogIndex;  // U11.4: cycle scan no longer walks the full catalog.

	// Resolve this Planner's direct children — the candidate operators it would
	// pass to A* if it ran a search at its own tier.
	using ActionHandle = FCk_Handle_Goap_Action;
	auto DirectChildren = TArray<ActionHandle>{};

	if (InHandle.template Has<FFragment_Goap_Action_Tree>())
	{
		// Promoted Action-Planner: direct children are this Action's children.
		const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
		DirectChildren = Tree.Get_ChildActions();
	}
	else if (auto RootAction = InCurrent.Get_RootAction(); ck::IsValid(RootAction))
	{
		// Top-level Planner: direct children are its root Action's children.
		const auto& RootTree = RootAction.template Get<FFragment_Goap_Action_Tree>();
		DirectChildren = RootTree.Get_ChildActions();
	}

	if (DirectChildren.IsEmpty())
	{
		InCurrent._DependencyCycles.Reset();
		InHandle.Remove<FTag_Goap_Planner_RequiresSetup>();
		return;
	}

	// Defer if any direct child hasn't completed Setup yet — we need its
	// `_Definition` (preconditions / effects) populated for the cycle-condition
	// overlap pass below.
	for (const auto& Child : DirectChildren)
	{
		if (NOT ck::IsValid(Child)) { continue; }
		if (Child.template Has<FTag_Goap_Action_RequiresSetup>())
		{
			return;  // Defer; keep the Planner's RequiresSetup tag for retry.
		}
	}

	// Build the adjacency map over the precondition/effect dependency graph.
	// Edge A -> B exists iff some effect (Key,Value) in A satisfies some
	// precondition (Key,Value) in B — i.e. B depends on A having run.
	//
	// Tracks the (Key) tags that closed each edge so non-trivial SCCs can
	// surface the participating WS keys as a diagnostic.
	auto Adj = TMap<ActionHandle, TArray<ActionHandle>>{};

	// Per-edge condition tags: edge(A,B) -> set of tags via which the edge was
	// established. Used to compute the cycle's participating keys after SCC.
	auto EdgeConditions = TMap<TPair<ActionHandle, ActionHandle>, TSet<FGameplayTag>>{};

	for (const auto& A : DirectChildren)
	{
		if (NOT ck::IsValid(A)) { continue; }
		auto& Edges = Adj.Add(A);

		const auto& DefA = A.template Get<FFragment_Goap_Action_Definition>();
		const auto& EffectsA = DefA.Get_Effects();
		if (EffectsA.IsEmpty()) { continue; }

		for (const auto& B : DirectChildren)
		{
			if (NOT ck::IsValid(B)) { continue; }

			const auto& DefB = B.template Get<FFragment_Goap_Action_Definition>();
			const auto& PreconditionsB = DefB.Get_Preconditions();
			if (PreconditionsB.IsEmpty()) { continue; }

			auto MatchedKeys = TSet<FGameplayTag>{};
			for (const auto& Eff : EffectsA)
			{
				for (const auto& Pre : PreconditionsB)
				{
					if (Eff.Key == Pre.Key && Eff.Value == Pre.Value)
					{
						MatchedKeys.Add(Eff.Key);
					}
				}
			}

			if (MatchedKeys.Num() > 0)
			{
				Edges.AddUnique(B);
				EdgeConditions.Add(TPair<ActionHandle, ActionHandle>{A, B}, MoveTemp(MatchedKeys));
			}
		}
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

		const auto SccNodeSet = TSet<ActionHandle>{Scc};

		for (const auto& ActionHandleInScc : Scc)
		{
			if (NOT ck::IsValid(ActionHandleInScc)) { continue; }

			const auto& Params = ActionHandleInScc.template Get<FFragment_Goap_Action_Params>();
			ActionsInCycle.Add(Params.Get_ActionClass());
		}

		// Collect the (Key) tags participating in the cycle: the union of every
		// intra-SCC edge's matched keys. These are the WS keys that close the
		// loop (A's effect on Key satisfies B's precondition on Key, etc.).
		auto CycleConditionsSet = TSet<FGameplayTag>{};
		for (const auto& Src : Scc)
		{
			const auto* Edges = Adj.Find(Src);
			if (Edges == nullptr) { continue; }

			for (const auto& Dst : *Edges)
			{
				if (NOT SccNodeSet.Contains(Dst)) { continue; }

				if (const auto* Tags = EdgeConditions.Find(TPair<ActionHandle, ActionHandle>{Src, Dst}))
				{
					for (const auto& Tag : *Tags) { CycleConditionsSet.Add(Tag); }
				}
			}
		}

		auto CycleConditions = CycleConditionsSet.Array();

		InCurrent._DependencyCycles.Add(
			FCk_GoapDiagnostic_DependencyCycle{MoveTemp(ActionsInCycle), MoveTemp(CycleConditions)});
	}

	if (InCurrent._DependencyCycles.Num() > 0)
	{
		ck::goap::Warning(
			TEXT("Planner [{}] has [{}] dependency cycle(s) among its direct children."),
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
// AddAction's first-call goal propagation or at runtime via Request_SetGoal).
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
	// of every Action is the top-level Planner entity — see AddAction's tree-
	// wiring branches in CkGoap_Planner_Utils.cpp).
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

	// PR-B.1b Stage 0 — broadcast still happens on the Action entity (Path A);
	// payload source-handle is the Planner-cast of the activated entity (sub-
	// Planners are always promoted, so this cast is safe).
	{
		auto PlannerCast = UCk_Utils_Goap_Planner_UE::Has(InPlanner)
			? UCk_Utils_Goap_Planner_UE::CastChecked(InPlanner)
			: FCk_Handle_Goap_Planner{};
		UUtils_Signal_OnGoap_Planner_Activated::Broadcast(
			InPlanner, ck::MakePayload(PlannerCast, FCk_Goap_Payload_OnPlannerActivated{}));
	}
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

	// PR-B.1b Stage 0 — broadcast still happens on the Action entity (Path A);
	// payload source-handle is the Planner-cast of the deactivated entity.
	{
		auto PlannerCast = UCk_Utils_Goap_Planner_UE::Has(InPlanner)
			? UCk_Utils_Goap_Planner_UE::CastChecked(InPlanner)
			: FCk_Handle_Goap_Planner{};
		UUtils_Signal_OnGoap_Planner_Deactivated::Broadcast(
			InPlanner, ck::MakePayload(PlannerCast, FCk_Goap_Payload_OnPlannerDeactivated{}));
	}
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
	// activated at AddAction time (implicit root); mid-tier sub-Planners are activated by
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
