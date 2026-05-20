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
// SETUP — Dependency-cycle detection on the ActionSet's Action catalog.
//
// TODO(U3): The old DFS used Plan[0]._ActionTag → tier _TierTag matching.
// In the unified model, dependency edges follow parent/child Action_Tree
// relationships and are checked at AddAction_ToAction time. This processor
// will be reduced or repurposed in U3. For now, body is stubbed to a no-op
// that only clears the RequiresSetup tag.
// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_ActionSet_Current& InCurrent,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) -> void
{
	// TODO(U3): rewrite per ActionSetUnification spec §4.2 — cycle detection
	// will run over the Action_Tree edges instead of the legacy ActionTag
	// matching.
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

	InHandle.Remove<FTag_Goap_ActionSet_RequiresSetup>();
	InCurrent._DependencyCycles.Reset();
}

// ====================================================================================================================
// HELPERS — static members of FProcessor_Goap_ActionSet_ChainUpdate so they
// inherit the processor's friend access to FFragment_Goap_Action_Current.
//
// TODO(U3): full rewrite per ActionSetUnification spec §4.2 — both helpers
// will be reimplemented when ChainUpdate is rewritten in U3.
// ====================================================================================================================

auto
	FProcessor_Goap_ActionSet_ChainUpdate::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InParentAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Action& InChildAction,
		FFragment_Goap_Action_Current& InChildCurrent) -> void
{
	// TODO(U3): rewrite per ActionSetUnification spec §4.2 — in the unified
	// model, goal injection reads from the parent Action_Definition's
	// _GoalFromEffects (already resolved at Setup) instead of round-tripping
	// through two key registries.
	InChildCurrent._Goal.Reset();
	InChildCurrent._InvalidGoal.Reset();
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
		auto& Current = Action.Get<FFragment_Goap_Action_Current>();

		// Unsubscribe the action from its resolved WS before tearing down state.
		if (ck::IsValid(Current._WorldStateSource_Resolved))
		{
			auto WS = Current._WorldStateSource_Resolved;
			::UCk_Utils_Goap_WorldState_UE::Request_RemoveSubscriber(WS, Action);
		}

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
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionSet_Params& InParams,
		const FFragment_Goap_ActionSet_Current& InCurrent,
		FFragment_Goap_ActionSet_ActiveChain& InActiveChain,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) const -> void
{
	// TODO(U3): rewrite per ActionSetUnification spec §4.2.
	// The old logic walked Plan[0]._ActionTag → tier-catalog lookup → append.
	// In the unified model, ChainUpdate will:
	//   1. Read each current chain Action's Plan[0] handle directly (Plan is
	//      now TArray<FCk_Handle_Goap_Action>).
	//   2. Append that next-Action to the chain if it differs from the
	//      existing child.
	//   3. Use the parent's Action_Definition._CachedActionDef + parent's
	//      Action_Tree._ChildActions as the planner's candidate operator set.
	// The legacy body referenced TierCatalogIndex, _TierTag, and
	// _Tier_ActionClasses, none of which fit the new model.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	(void)InParams;
	(void)InActiveChain;
	(void)InCatalogIndex;
	(void)InHandle;
}

// ====================================================================================================================

} // namespace ck
