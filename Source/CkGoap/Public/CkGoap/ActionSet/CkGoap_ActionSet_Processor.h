#pragma once

#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — ActionSet-scoped: detect Action dependency cycles in the catalog.
// Runs once whenever the ActionSet's RequiresSetup tag is set. Defers if any
// catalog Action hasn't completed its own Setup yet.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_ActionSet_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_ActionSet_Setup,
	FCk_Handle_Goap_ActionSet,
	ck::TReadWrite<FFragment_Goap_ActionSet_Current>,
	ck::TReadOnly<FFragment_Goap_ActionSet_ActionCatalogIndex>,
	FTag_Goap_ActionSet_RequiresSetup,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;

public:
	using TProcessor::TProcessor;

public:
	static auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		FFragment_Goap_ActionSet_Current& InCurrent,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) -> void;
};

// ====================================================================================================================
// CHAIN UPDATE — Walk each ActionSet's ActiveChain; apply truncate / extend
// rule based on each Action's Plan[0]. Runs LAST in the AI group so all
// Actions have planned for the frame before chain mutation.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_ActionSet_ChainUpdate : public ck_exp::TProcessor<
	FProcessor_Goap_ActionSet_ChainUpdate,
	FCk_Handle_Goap_ActionSet,
	ck::TReadOnly<FFragment_Goap_ActionSet_Params>,
	ck::TReadOnly<FFragment_Goap_ActionSet_Current>,
	ck::TReadWrite<FFragment_Goap_ActionSet_ActiveChain>,
	ck::TReadOnly<FFragment_Goap_ActionSet_ActionCatalogIndex>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Action_HandleResult>;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionSet_Params& InParams,
		const FFragment_Goap_ActionSet_Current& InCurrent,
		FFragment_Goap_ActionSet_ActiveChain& InActiveChain,
		const FFragment_Goap_ActionSet_ActionCatalogIndex& InCatalogIndex) const -> void;

private:
	// These helpers mutate private members of FFragment_Goap_Action_Current and
	// FFragment_Goap_WorldState_Subscribers. Declared as static members so they
	// inherit the processor's friend access to both fragments.
	static auto
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Action& InParentAction,
		TSubclassOf<class UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Action& InChildAction,
		FFragment_Goap_Action_Current& InChildCurrent) -> void;

	static auto
	DoTruncateChainFrom(
		TArray<FCk_Handle_Goap_Action>& InActiveChain,
		int32 InStartIndex) -> void;

	// Walks the WS-source override chain: child's _WorldStateSource_Override →
	// parent's _WorldStateSource_Resolved → ActionSet's _WorldStateSource.
	// Writes the resolved source into the child's _WorldStateSource_Resolved.
	static auto
	DoResolveAndAssignWorldStateSource(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParent,
		const FCk_Handle_Goap_ActionSet& InActionSet) -> void;

	// Add/remove an Action's handle to/from its resolved WS's subscriber list.
	static auto
	DoSubscribeActionToWorldState(FCk_Handle_Goap_Action& InAction) -> void;

	static auto
	DoUnsubscribeActionFromWorldState(FCk_Handle_Goap_Action& InAction) -> void;
};

// ====================================================================================================================

} // namespace ck
