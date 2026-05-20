#include "CkGoap/ActionSet/CkGoap_ActionSet_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_ActionSet_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_ActionSet_ChainUpdate);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — Cycle detection on the ActionSet's Action catalog.
//
// TODO(U5): cycle detection via iterative Tarjan SCC over _ChildActions edges.
// For now we just clear the diagnostics list and remove the setup tag once
// every catalog Action has completed its own Setup.
// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_ActionSet_Current& InCurrent,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) -> void
{
	const auto& Catalog = InCatalogIndex.Get_TagToAction();
	if (Catalog.IsEmpty())
	{
		InHandle.Remove<FTag_Goap_ActionSet_RequiresSetup>();
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

	// TODO(U5): cycle detection via iterative Tarjan SCC over _ChildActions
	// edges. For now we just clear the diagnostics list and remove the setup tag.
	InCurrent._DependencyCycles.Reset();
	InHandle.Remove<FTag_Goap_ActionSet_RequiresSetup>();
}

// ====================================================================================================================
// HELPERS — static members of FProcessor_Goap_ActionSet_ChainUpdate so they
// inherit the processor's friend access to FFragment_Goap_Action_Current.
// ====================================================================================================================

// Inject the child Action's goal from the CHILD's own pre-resolved
// _GoalFromEffects (built once at Setup from the Action's own Effects). The
// authored tag-keyed entries are resolved against the child's already-assigned
// _WorldStateSource_Resolved.
//
// Precondition: InChildCurrent._WorldStateSource_Resolved is valid.
auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InParentAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Action& InChildAction,
		FFragment_Goap_Action_Current& InChildCurrent) -> void
{
	(void)InParentAction;
	(void)InParentActionClass;

	InChildCurrent._Goal.Reset();
	InChildCurrent._InvalidGoal.Reset();

	const auto& ChildDef = InChildAction.template Get<FFragment_Goap_Action_Definition>();
	const auto& GoalFromEffects = ChildDef.Get_GoalFromEffects();

	// TODO(U5): _InvalidGoal will be populated at Setup; copying it here will
	// surface effect-keys-not-in-registry diagnostics. For now Setup leaves it
	// empty (raw copy without registry validation).
	InChildCurrent._InvalidGoal = ChildDef.Get_InvalidGoal();

	const auto& WS = InChildCurrent._WorldStateSource_Resolved;
	if (NOT ck::IsValid(WS))
	{
		// Can't resolve keys without a WS — record everything as invalid.
		for (const auto& Authored : GoalFromEffects)
		{
			InChildCurrent._InvalidGoal.Add(Authored);
		}
		return;
	}

	const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(WS)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();

	InChildCurrent._Goal.Reserve(GoalFromEffects.Num());
	for (const auto& Authored : GoalFromEffects)
	{
		const auto Key = Registry.Find(Authored.Get_Key());
		if (Key == goap::InvalidGoapKey)
		{
			InChildCurrent._InvalidGoal.Add(Authored);
			continue;
		}
		InChildCurrent._Goal.Add(goap::FWorldStateCondition{Key, Authored.Get_Value()});
	}
}

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
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

		auto& Current = Action.Get<FFragment_Goap_Action_Current>();
		Current._Goal.Reset();
		Current._InvalidGoal.Reset();
		Current._ActiveParentAction = nullptr;
		Current._WorldStateSource_Resolved = {};
		Current._Plan.Reset();
		Current._PlanStatus = ECk_GoapPlanStatus::Idle;

		UUtils_Signal_OnGoap_Action_Deactivated::Broadcast(
			Action, ck::MakePayload(Action, FCk_Goap_Payload_OnActionDeactivated{}));

		InActiveChain.RemoveAt(i);
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent,
		const FCk_Handle_Goap_ActionSet& InActionSet) -> void
{
	auto& ChildCurrent = InChild.template Get<FFragment_Goap_Action_Current>();
	const auto& ChildParams = InChild.template Get<FFragment_Goap_Action_Params>();

	// 1. Child's own override (if set) wins.
	const auto Override = ChildParams.Get_WorldStateSource_Override();
	if (ck::IsValid(Override))
	{
		ChildCurrent._WorldStateSource_Resolved = Override;
		return;
	}

	// 2. Inherit from the parent's resolved WS.
	if (ck::IsValid(InParent))
	{
		const auto& ParentCurrent = InParent.template Get<FFragment_Goap_Action_Current>();
		if (ck::IsValid(ParentCurrent.Get_WorldStateSource_Resolved()))
		{
			ChildCurrent._WorldStateSource_Resolved = ParentCurrent.Get_WorldStateSource_Resolved();
			return;
		}
	}

	// 3. Fall back to the ActionSet-level default WS source.
	if (InActionSet.template Has<FFragment_Goap_ActionSet_WorldStateSource>())
	{
		const auto& SetWS = InActionSet.template Get<FFragment_Goap_ActionSet_WorldStateSource>();
		ChildCurrent._WorldStateSource_Resolved = SetWS.Get_WorldStateSource();
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	auto& Current = InAction.template Get<FFragment_Goap_Action_Current>();
	auto WS = Current._WorldStateSource_Resolved;
	if (NOT ck::IsValid(WS)) { return; }

	auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.AddUnique(FCk_Handle{InAction});
}

// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void
{
	auto& Current = InAction.template Get<FFragment_Goap_Action_Current>();
	auto WS = Current._WorldStateSource_Resolved;
	if (NOT ck::IsValid(WS)) { return; }

	auto& Subscribers = WS.template Get<FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.RemoveSwap(FCk_Handle{InAction});
}

// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionSet_Params& InParams,
		const FFragment_Goap_ActionSet_Current& InCurrent,
		FFragment_Goap_ActionSet_ActiveChain& InActiveChain,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) const -> void
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

		const auto& CurrCurrent = CurrAction.template Get<FFragment_Goap_Action_Current>();

		if (CurrCurrent.Get_PlanStatus() != ECk_GoapPlanStatus::PlanFound)
		{
			// Mid-decision (Idle / Planning / PlanFailed / CostThresholdReached).
			// Don't mutate the chain — wait for this Action to settle.
			break;
		}

		const auto& Plan = CurrCurrent.Get_Plan();

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
		DoInjectGoalSynchronous(CurrAction, CurrParams.Get_ActionClass(), NextChild, ChildCurrent);
		DoSubscribeActionToWorldState(NextChild);

		NextChild.template AddOrGet<FTag_Goap_Action_RequiresInitialPlan>();

		UUtils_Signal_OnGoap_Action_Activated::Broadcast(
			NextChild, ck::MakePayload(NextChild, FCk_Goap_Payload_OnActionActivated{}));

		// New tier; it won't have planned yet this frame so stop here.
		break;
	}

	if (bChainChanged)
	{
		UUtils_Signal_OnGoap_ActionSet_ActiveChainChanged::Broadcast(
			InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnActiveChainChanged{OldChainSnapshot}));
	}
}

// ====================================================================================================================

} // namespace ck
