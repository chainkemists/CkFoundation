#include "CkGoap/Planner/CkGoap_Planner_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_ChainUpdate);

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
// HELPERS — static members of FProcessor_Goap_Planner_ChainUpdate so they
// inherit the processor's friend access to FFragment_Goap_Action_Current.
// ====================================================================================================================

// Inject the child Action's goal from the CHILD's own pre-resolved
// _GoalFromEffects (built once at Setup from the Action's own Effects). The
// authored tag-keyed entries are resolved against the child's already-assigned
// resolved WS source.
//
// Precondition: child's FFragment_Goap_Planner_WorldStateSource._Resolved is valid.
auto
	FProcessor_Goap_Planner_ChainUpdate::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InParentAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Action& InChildAction,
		FFragment_Goap_Planner_Goal& InChildGoal,
		const FFragment_Goap_Planner_WorldStateSource& InChildWSSource) -> void
{
	(void)InParentAction;
	(void)InParentActionClass;

	InChildGoal._Goal.Reset();
	InChildGoal._InvalidGoal.Reset();

	const auto& ChildDef = InChildAction.template Get<FFragment_Goap_Action_Definition>();
	const auto& GoalFromEffects = ChildDef.Get_GoalFromEffects();

	// U5.1: Action_Definition._InvalidGoal is populated at Setup time with any
	// effect tag missing from the Action's resolved WS registry. Seed the
	// Goal fragment's _InvalidGoal from that — chain-activation may add more
	// entries below if the parent's effects (used as our goal) aren't
	// registered here.
	InChildGoal._InvalidGoal = ChildDef.Get_InvalidGoal();

	const auto& WS = InChildWSSource.Get_Resolved();
	if (NOT ck::IsValid(WS))
	{
		// Can't resolve keys without a WS — record everything as invalid.
		for (const auto& Authored : GoalFromEffects)
		{
			InChildGoal._InvalidGoal.Add(Authored);
		}
		return;
	}

	const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(WS)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();

	InChildGoal._Goal.Reserve(GoalFromEffects.Num());
	for (const auto& Authored : GoalFromEffects)
	{
		const auto Key = Registry.Find(Authored.Get_Key());
		if (Key == goap::InvalidGoapKey)
		{
			InChildGoal._InvalidGoal.Add(Authored);
			continue;
		}
		InChildGoal._Goal.Add(goap::FWorldStateCondition{Key, Authored.Get_Value()});
	}
}

auto
	FProcessor_Goap_Planner_ChainUpdate::
	DoTruncateChainFrom(
		TArray<FCk_Handle_Goap_Action>& InActiveChain,
		int32 InStartIndex) -> void
{
	for (auto i = InActiveChain.Num() - 1; i >= InStartIndex; --i)
	{
		auto& Action = InActiveChain[i];

		// Unsubscribe synchronously (avoids a deferred request that would land
		// after the next chain update on this same Action).
		DoUnsubscribeActionFromWorldState(Action);

		auto& Current   = Action.Get<FFragment_Goap_Action_Current>();
		auto& Goal      = Action.Get<FFragment_Goap_Planner_Goal>();
		auto& PlanState = Action.Get<FFragment_Goap_Planner_PlanState>();
		auto& WSSource  = Action.Get<FFragment_Goap_Planner_WorldStateSource>();

		Goal._Goal.Reset();
		Goal._InvalidGoal.Reset();
		Current._ActiveParentAction = nullptr;
		WSSource._Resolved = {};
		PlanState._Plan.Reset();
		PlanState._PlanStatus = ECk_GoapPlanStatus::Idle;

		// Release any in-flight gating tag — Action is leaving the chain and
		// won't broadcast a terminal status to drop the tag itself.
		Action.template Try_Remove<FTag_Goap_Action_PlanInFlight>();

		UUtils_Signal_OnGoap_Action_Deactivated::Broadcast(
			Action, ck::MakePayload(Action, FCk_Goap_Payload_OnActionDeactivated{}));

		InActiveChain.RemoveAt(i);
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_ChainUpdate::
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent,
		const FCk_Handle_Goap_Planner& InPlanner) -> void
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

	// 3. Fall back to the ActionSet-level default WS source.
	if (InPlanner.template Has<FFragment_Goap_Planner_WorldStateSource>())
	{
		const auto& SetWS = InPlanner.template Get<FFragment_Goap_Planner_WorldStateSource>();
		ChildWSSource._Resolved = SetWS.Get_WorldStateSource();
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_ChainUpdate::
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
	FProcessor_Goap_Planner_ChainUpdate::
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	auto& WSSource = InAction.template Get<FFragment_Goap_Planner_WorldStateSource>();
	auto WS = WSSource._Resolved;
	if (NOT ck::IsValid(WS)) { return; }

	auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.RemoveSwap(FCk_Handle{InAction});
}

// ====================================================================================================================

auto
	FProcessor_Goap_Planner_ChainUpdate::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		FFragment_Goap_Planner_ActiveChain& InActiveChain,
		const FFragment_Goap_Planner_ActionCatalogIndex& InCatalogIndex) const -> void
{
	(void)InParams;
	(void)InCatalogIndex;

	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	auto& Chain = InActiveChain._Chain;
	if (Chain.IsEmpty()) { return; }

	const auto OldChainSnapshot = Chain;
	auto bChainChanged = false;

	auto i = 0;
	while (i < Chain.Num())
	{
		auto CurrAction = Chain[i];
		if (NOT ck::IsValid(CurrAction))
		{
			// Stale handle — drop and continue with the truncated chain.
			DoTruncateChainFrom(Chain, i);
			bChainChanged = true;
			break;
		}

		const auto& CurrPlanState = CurrAction.template Get<FFragment_Goap_Planner_PlanState>();

		if (CurrPlanState.Get_PlanStatus() != ECk_GoapPlanStatus::PlanFound)
		{
			// Mid-decision (Idle / Planning / PlanFailed / CostThresholdReached).
			// Don't mutate the chain — wait for this Action to settle.
			break;
		}

		const auto& Plan = CurrPlanState.Get_Plan();

		if (Plan.IsEmpty())
		{
			// Goal already satisfied (or no actions chosen). Anything past i
			// is stale and must be torn down.
			if (i + 1 < Chain.Num())
			{
				DoTruncateChainFrom(Chain, i + 1);
				bChainChanged = true;
			}
			break;
		}

		auto NextChild = Plan[0];
		if (NOT ck::IsValid(NextChild))
		{
			// Defensive: planner emitted an invalid handle — skip extension.
			break;
		}

		if (i + 1 < Chain.Num())
		{
			const auto& ExistingChild = Chain[i + 1];
			if (ExistingChild == NextChild)
			{
				// Existing chain entry still matches — advance.
				++i;
				continue;
			}

			// Different child — truncate the divergent tail.
			DoTruncateChainFrom(Chain, i + 1);
			bChainChanged = true;
		}

		// Atomic vs composite — only composite Actions get appended to the
		// chain (atomic Actions have no children to plan over).
		const auto& ChildTree = NextChild.template Get<FFragment_Goap_Action_Tree>();
		if (ChildTree.Get_ChildActions().IsEmpty())
		{
			// Atomic — chain terminates here; consumer drives the leaf Action.
			break;
		}

		// Composite — append to the chain and activate.
		Chain.Add(NextChild);
		bChainChanged = true;

		auto& ChildCurrent = NextChild.template Get<FFragment_Goap_Action_Current>();
		const auto& CurrParams = CurrAction.template Get<FFragment_Goap_Action_Params>();
		ChildCurrent._ActiveParentAction = CurrParams.Get_ActionClass();

		// Resolve WS first so DoInjectGoalSynchronous can use the registry.
		DoResolveAndAssignWorldStateSource(NextChild, CurrAction, InHandle);
		auto& ChildGoal     = NextChild.template Get<FFragment_Goap_Planner_Goal>();
		const auto& ChildWS = NextChild.template Get<FFragment_Goap_Planner_WorldStateSource>();
		DoInjectGoalSynchronous(CurrAction, CurrParams.Get_ActionClass(), NextChild, ChildGoal, ChildWS);
		DoSubscribeActionToWorldState(NextChild);

		NextChild.template AddOrGet<FTag_Goap_Action_RequiresInitialPlan>();

		UUtils_Signal_OnGoap_Action_Activated::Broadcast(
			NextChild, ck::MakePayload(NextChild, FCk_Goap_Payload_OnActionActivated{}));

		// New tier; it won't have planned yet this frame so stop here.
		break;
	}

	if (bChainChanged)
	{
		UUtils_Signal_OnGoap_Planner_ActiveChainChanged::Broadcast(
			InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnActiveChainChanged{OldChainSnapshot}));
	}
}

// ====================================================================================================================

} // namespace ck
