#include "CkGoap/Planner/CkGoap_Planner_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Stats.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Utils.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/Planner/CkGoap_Planner_Internal.h"  // shared WS-source resolver
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_UpdateActivation);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("GoapPlanner::Setup"), STAT_Goap_Planner_Setup, STATGROUP_CkGoap);
DECLARE_CYCLE_STAT(TEXT("GoapPlanner::UpdateActivation"), STAT_Goap_Planner_UpdateActivation, STATGROUP_CkGoap);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_CkGoap_Planner_setup_internal
{
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
			if (Index.Contains(Root))
			{ continue; }

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

				const auto NodeHandle = Frame.Node;
				if (Lowlink[NodeHandle] == Index[NodeHandle])
				{
					auto Scc = TArray<HandleT>{};
					while (true)
					{
						const auto Popped = Stack.Pop();
						OnStack.Remove(Popped);
						Scc.Add(Popped);
						if (Popped == NodeHandle)
						{ break; }
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

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_ActionCatalogIndex& InCatalogIndex,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_Goal& InGoal) -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_Planner_Setup);

	using ActionHandle = FCk_Handle_Goap_Action;
	auto DirectChildren = TArray<ActionHandle>{};

	const auto IsPromotedMidTierPlanner = InHandle.template Has<FFragment_Goap_Action_Tree>();
	if (IsPromotedMidTierPlanner)
	{
		const auto& Tree = InHandle.template Get<FFragment_Goap_Action_Tree>();
		DirectChildren = Tree.Get_ChildActions();
	}
	else
	{
		DirectChildren.Reserve(InCatalogIndex.Get_TagToAction().Num());
		for (const auto& Pair : InCatalogIndex.Get_TagToAction())
		{
			if (ck::IsValid(Pair.Value))
			{ DirectChildren.Add(Pair.Value); }
		}
	}

	if (DirectChildren.IsEmpty())
	{
		InCurrent._DependencyCycles.Reset();
		InHandle.Remove<FTag_Goap_Planner_RequiresSetup>();
		return;
	}

	for (const auto& Child : DirectChildren)
	{
		if (ck::Is_NOT_Valid(Child))
		{ continue; }
		if (Child.template Has<FTag_Goap_Action_RequiresSetup>())
		{
			return;  // Defer, keeping RequiresSetup: the overlap pass below needs the child's _Definition.
		}
	}

	// Edge A -> B iff some effect (Key,Value) of A satisfies some precondition (Key,Value) of B.
	auto Adj = TMap<ActionHandle, TArray<ActionHandle>>{};
	auto EdgeConditions = TMap<TPair<ActionHandle, ActionHandle>, TSet<FGameplayTag>>{};

	for (const auto& A : DirectChildren)
	{
		if (ck::Is_NOT_Valid(A))
		{ continue; }
		auto& Edges = Adj.Add(A);

		const auto& DefA = A.template Get<FFragment_Goap_Action_Definition>();
		const auto& EffectsA = DefA.Get_Effects();
		if (EffectsA.IsEmpty())
		{ continue; }

		for (const auto& B : DirectChildren)
		{
			if (ck::Is_NOT_Valid(B))
			{ continue; }

			const auto& DefB = B.template Get<FFragment_Goap_Action_Definition>();
			const auto& PreconditionsB = DefB.Get_Preconditions();
			if (PreconditionsB.IsEmpty())
			{ continue; }

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
		if (Scc.Num() == 1)
		{
			const auto* Edges = Adj.Find(Scc[0]);
			const auto HasSelfLoop = (Edges != nullptr) && Edges->Contains(Scc[0]);
			if (NOT HasSelfLoop)
			{ continue; }
		}

		auto ActionsInCycle = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};
		ActionsInCycle.Reserve(Scc.Num());

		const auto SccNodeSet = TSet<ActionHandle>{Scc};

		for (const auto& ActionHandleInScc : Scc)
		{
			if (ck::Is_NOT_Valid(ActionHandleInScc))
			{ continue; }

			const auto& Params = ActionHandleInScc.template Get<FFragment_Goap_Action_Params>();
			ActionsInCycle.Add(Params.Get_ActionClass());
		}

		auto CycleConditionsSet = TSet<FGameplayTag>{};
		for (const auto& Src : Scc)
		{
			const auto* Edges = Adj.Find(Src);
			if (Edges == nullptr)
			{ continue; }

			for (const auto& Dst : *Edges)
			{
				if (NOT SccNodeSet.Contains(Dst))
				{ continue; }

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

	// Keys absent from the registry are silently dropped here; Request_SetGoal owns _InvalidGoal,
	// the canonical diagnostic surface. Deliberately does NOT defer on an unresolved WS source.
	if (NOT InGoal._GoalAuthored.IsEmpty() && InGoal._Goal.IsEmpty())
	{
		const auto Source = InWSSource.Get_Resolved();
		if (ck::IsValid(Source))
		{
			auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
				.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

			for (const auto& Cond : InGoal._GoalAuthored)
			{
				const auto Key = Registry.Find(Cond.Get_Key());
				if (Key != goap::InvalidGoapKey)
				{
					InGoal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
				}
			}
		}
	}

	const auto HasUnconditionalFallback = [&]() -> bool
	{
		if (InGoal._GoalAuthored.IsEmpty())
		{ return true; }

		for (const auto& Child : DirectChildren)
		{
			if (ck::Is_NOT_Valid(Child))
			{ continue; }

			const auto& Def = Child.template Get<FFragment_Goap_Action_Definition>();
			if (NOT Def.Get_Preconditions().IsEmpty())
			{ continue; }

			const auto& Effects = Def.Get_Effects();
			auto AllGoalsCovered = true;
			for (const auto& GoalCond : InGoal._GoalAuthored)
			{
				auto Covered = false;
				for (const auto& Eff : Effects)
				{
					if (Eff.Key == GoalCond.Get_Key() && Eff.Value == GoalCond.Get_Value())
					{
						Covered = true;
						break;
					}
				}
				if (NOT Covered)
				{ AllGoalsCovered = false; break; }
			}
			if (AllGoalsCovered)
			{ return true; }
		}
		return false;
	}();

	InCurrent._HasUnconditionalFallback = HasUnconditionalFallback;

	if (NOT HasUnconditionalFallback && NOT InParams.Get_AllowPlanFailed())
	{
		CK_ENSURE_IF_NOT(false,
			TEXT("Planner [{}] (tag [{}]) has no unconditional fallback Action that satisfies its goal — "
				 "the catalog contains no Action with empty preconditions whose effects cover every goal "
				 "condition. PlanFailed is therefore reachable from some world-state, but "
				 "PlannerParams._AllowPlanFailed=false. Add a fallback Action (no preconditions, "
				 "effect=goal, cost ~999.0) such as WaitForEnemy / StandWatch / Idle, OR set "
				 "_AllowPlanFailed=true if this Planner is intentionally allowed to PlanFail (tests, "
				 "research catalogs, gyms that demonstrate PlanFailed)."),
			InHandle, InParams.Get_PlannerTag())
		{ /* still proceed; ensure surfaces but doesn't block setup */ }
	}

	InHandle.Remove<FTag_Goap_Planner_RequiresSetup>();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_goap_planner_processor
{
	auto Is_Composite(const FCk_Handle_Goap_Action& InAction) -> bool
	{
		if (ck::Is_NOT_Valid(InAction))
		{ return false; }
		const auto& Tree = InAction.template Get<FFragment_Goap_Action_Tree>();
		return NOT Tree.Get_ChildActions().IsEmpty();
	}
}

// Re-resolves even though Setup already did: a parent-inherited WS source may carry a different
// registry than the one current at Setup time.
auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InPlanner) -> void
{
	auto& Goal = InPlanner.template Get<FFragment_Goap_Planner_Goal>();
	const auto& WSSource = InPlanner.template Get<FFragment_Goap_Planner_WorldStateSource>();

	Goal._Goal.Reset();
	Goal._InvalidGoal.Reset();

	// The Action-role effect-key diagnostics stay surfaced here: they describe this entity as a
	// candidate operator for its parent's planner, independent of its own goal.
	const auto& Def = InPlanner.template Get<FFragment_Goap_Action_Definition>();
	Goal._InvalidGoal = Def.Get_InvalidGoal();

	const auto& Authored = Goal.Get_GoalAuthored();
	if (Authored.IsEmpty())
	{
		// No goal authored — the planner emits an empty plan / PlanFound immediately.
		return;
	}

	const auto& WS = WSSource.Get_Resolved();
	if (ck::Is_NOT_Valid(WS))
	{
		for (const auto& Cond : Authored)
		{
			Goal._InvalidGoal.Add(Cond);
		}
		return;
	}

	// Residency-classified, not raw FindOrRegister: the planner's resolved WS may be a parented
	// sub-WS, and a goal key owned by an ancestor must become an import alias, not a local shadow.
	auto MutableWS = WS;
	Goal._Goal.Reserve(Authored.Num());
	for (const auto& Cond : Authored)
	{
		const auto Key = UCk_Utils_Goap_WorldState_UE::Register_Key_WithResidencyClassification(
			MutableWS, Cond.Get_Key());
		if (Key == goap::InvalidGoapKey)
		{
			Goal._InvalidGoal.Add(Cond);
			continue;
		}
		Goal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent) -> void
{
	ck::goap::internal_planner::DoResolveChildWorldStateFromParent(InChild, InParent,
		ck::goap::internal_planner::EResolveWorldStateSourcePolicy::Reassign);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	// The whole parent chain: an imported key's truth changes on an ancestor and must dirty us.
	auto& WSSource = InAction.template Get<FFragment_Goap_Planner_WorldStateSource>();
	auto WS = WSSource._Resolved;
	for (auto Depth = 0;
		ck::IsValid(WS) && Depth < UCk_Utils_Goap_WorldState_UE::MaxParentChainDepth;
		++Depth)
	{
		if (NOT WS.template Has<FFragment_Goap_WorldState_Subscribers>())
		{ return; }
		auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
		Subscribers._Subscribers.AddUnique(FCk_Handle{InAction});

		WS = UCk_Utils_Goap_WorldState_UE::Get_FallbackParent(WS);
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	// Mirror of the chain subscribe — a deactivated sub-planner left subscribed to an ancestor
	// would take spurious replan pressure from every shared-gate flip.
	auto& WSSource = InAction.template Get<FFragment_Goap_Planner_WorldStateSource>();
	auto WS = WSSource._Resolved;
	for (auto Depth = 0;
		ck::IsValid(WS) && Depth < UCk_Utils_Goap_WorldState_UE::MaxParentChainDepth;
		++Depth)
	{
		if (NOT WS.template Has<FFragment_Goap_WorldState_Subscribers>())
		{ return; }
		auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
		Subscribers._Subscribers.RemoveSwap(FCk_Handle{InAction});

		WS = UCk_Utils_Goap_WorldState_UE::Get_FallbackParent(WS);
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoActivatePlanner(
		FCk_Handle_Goap_Action InPlanner,
		FCk_Handle_Goap_Action InParent) -> void
{
	if (ck::Is_NOT_Valid(InPlanner))
	{ return; }

	auto& Activation = InPlanner.template Get<FFragment_Goap_Planner_Activation>();
	if (Activation._IsActive)
	{ return; }

	if (ck::IsValid(InParent))
	{
		auto& ChildCurrent = InPlanner.template Get<FFragment_Goap_Action_Current>();
		const auto& ParentParams = InParent.template Get<FFragment_Goap_Action_Params>();
		ChildCurrent._ActiveParentAction = ParentParams.Get_ActionClass();
	}

	// WS first: DoInjectGoalSynchronous resolves goal keys against its registry.
	DoResolveAndAssignWorldStateSource(InPlanner, InParent);
	DoInjectGoalSynchronous(InPlanner);
	DoSubscribeActionToWorldState(InPlanner);

	InPlanner.template AddOrGet<FTag_Goap_Planner_RequiresInitialPlan>();
	goap::MarkReplanCandidate(InPlanner);

	Activation._IsActive = true;
	goap::MarkActivationDirty(InPlanner);

	{
		auto PlannerCast = UCk_Utils_Goap_Planner_UE::Has(InPlanner)
			? UCk_Utils_Goap_Planner_UE::CastChecked(InPlanner)
			: FCk_Handle_Goap_Planner{};
		UUtils_Signal_OnGoap_Planner_Activated::Broadcast(
			InPlanner, ck::MakePayload(PlannerCast, FCk_Goap_Payload_OnPlannerActivated{}));
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	DoDeactivatePlanner(FCk_Handle_Goap_Action InPlanner) -> void
{
	if (ck::Is_NOT_Valid(InPlanner))
	{ return; }

	auto& Activation = InPlanner.template Get<FFragment_Goap_Planner_Activation>();
	if (NOT Activation._IsActive)
	{
		Activation._LastActivatedPlan0 = {};
		return;
	}

	// Recurse through the CACHED plan0, not PlanState._Plan[0]: the live plan may already have
	// mutated (that mutation is what triggered this deactivation). Deeper layers settle first.
	{
		auto LastDescendant = Activation._LastActivatedPlan0;
		if (ck::IsValid(LastDescendant) && ck_goap_planner_processor::Is_Composite(LastDescendant))
		{
			DoDeactivatePlanner(LastDescendant);
		}
	}

	// Synchronous: a deferred request would land after the next activation pass on this Action.
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
	goap::MarkActivationDirty(InPlanner);

	// A Planner leaving the chain never broadcasts a terminal status, so nothing else drops these.
	InPlanner.template Try_Remove<FTag_Goap_Planner_RequiresInitialPlan>();
	InPlanner.template Try_Remove<FTag_Goap_Planner_PlanInFlight>();

	Activation._IsActive = false;
	Activation._LastActivatedPlan0 = {};

	{
		auto PlannerCast = UCk_Utils_Goap_Planner_UE::Has(InPlanner)
			? UCk_Utils_Goap_Planner_UE::CastChecked(InPlanner)
			: FCk_Handle_Goap_Planner{};
		UUtils_Signal_OnGoap_Planner_Deactivated::Broadcast(
			InPlanner, ck::MakePayload(PlannerCast, FCk_Goap_Payload_OnPlannerDeactivated{}));
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_UpdateActivation::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Activation& InActivation) const -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_Planner_UpdateActivation);

	if (NOT InActivation._IsActive)
	{
		InHandle.template Try_Remove<FTag_Goap_Planner_ActivationDirty>();
		return;
	}

	// The one path that KEEPS the tag: a disabled Planner must be revisited on re-enable, and the
	// enable toggle is not the only thing that can have changed while it sat disabled.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable)
	{ return; }

	const auto PlanStatus = InPlanState.Get_PlanStatus();
	const auto DecisionIsSettled = PlanStatus == ECk_GoapPlanStatus::PlanFound ||
		PlanStatus == ECk_GoapPlanStatus::PlanFailed;
	if (NOT DecisionIsSettled)
	{
		InHandle.template Try_Remove<FTag_Goap_Planner_ActivationDirty>();
		return;
	}

	const auto OldStep0 = InActivation._LastActivatedPlan0;
	const auto NewStep0 = InPlanState.Get_Plan().IsEmpty()
		? FCk_Handle_Goap_Action{}
		: InPlanState.Get_Plan()[0];

	if (OldStep0 == NewStep0)
	{
		InHandle.template Try_Remove<FTag_Goap_Planner_ActivationDirty>();
		return;
	}

	const auto IsPromotedMidTierPlanner = InHandle.template Has<FFragment_Goap_Action_Tree>();

	// Snapshot BEFORE mutating activation state so OnGoap_Planner_ActiveChainChanged can carry a
	// pre-mutation _OldChain payload. The chain is rooted at the top-level Planner.
	auto TopLevelPlanner = FCk_Handle_Goap_Planner{};
	auto OldChainSnapshot = TArray<FCk_Handle_Goap_Action>{};
	{
		if (NOT IsPromotedMidTierPlanner)
		{
			TopLevelPlanner = InHandle;
		}
		else
		{
			auto Walker = static_cast<FCk_Handle>(InHandle);
			constexpr auto MaxDepth = 64;
			for (auto Depth = 0; Depth < MaxDepth; ++Depth)
			{
				auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Walker);
				if (ck::Is_NOT_Valid(Owner))
				{ break; }
				if (UCk_Utils_Goap_Planner_UE::Has(Owner) &&
					NOT Owner.template Has<FFragment_Goap_Action_Tree>())
				{
					TopLevelPlanner = UCk_Utils_Goap_Planner_UE::CastChecked(Owner);
					break;
				}
				Walker = Owner;
			}
		}

		if (ck::IsValid(TopLevelPlanner))
		{
			OldChainSnapshot = UCk_Utils_Goap_Planner_UE::Get_ActiveChain(TopLevelPlanner);
		}
	}

	// A top-level Planner is not an Action, so its Plan[0] entries have no parent Action.
	auto ParentAsAction = FCk_Handle_Goap_Action{};
	if (IsPromotedMidTierPlanner)
	{
		ParentAsAction = UCk_Utils_Goap_Action_UE::CastChecked(InHandle);
	}

	if (ck::IsValid(OldStep0) && ck_goap_planner_processor::Is_Composite(OldStep0))
	{
		DoDeactivatePlanner(OldStep0);
	}

	if (ck::IsValid(NewStep0) && ck_goap_planner_processor::Is_Composite(NewStep0))
	{
		auto& ChildActivation = NewStep0.template Get<FFragment_Goap_Planner_Activation>();
		if (NOT ChildActivation._IsActive)
		{
			DoActivatePlanner(NewStep0, ParentAsAction);
		}
	}

	InActivation._LastActivatedPlan0 = NewStep0;

	if (ck::IsValid(TopLevelPlanner))
	{
		UUtils_Signal_OnGoap_Planner_ActiveChainChanged::Broadcast(
			TopLevelPlanner, ck::MakePayload(TopLevelPlanner,
				FCk_Goap_Payload_OnActiveChainChanged{OldChainSnapshot}));
	}

	InHandle.template Try_Remove<FTag_Goap_Planner_ActivationDirty>();
}

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
