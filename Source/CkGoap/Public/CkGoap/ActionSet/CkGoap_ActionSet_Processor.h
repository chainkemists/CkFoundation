#pragma once

#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/Tier/CkGoap_Tier_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — ActionSet-scoped: detect tier dependency cycles in the catalog.
// Runs once whenever the ActionSet's RequiresSetup tag is set. Defers if any
// catalog tier hasn't completed its own Setup yet.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_ActionSet_Setup : public ck_exp::TProcessor<
	FProcessor_Goap_ActionSet_Setup,
	FCk_Handle_Goap_ActionSet,
	ck::TReadWrite<FFragment_Goap_ActionSet_Current>,
	ck::TReadOnly<FFragment_Goap_ActionSet_TierCatalogIndex>,
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
		const FFragment_Goap_ActionSet_TierCatalogIndex& InCatalogIndex) -> void;
};

// ====================================================================================================================
// CHAIN UPDATE — Walk each ActionSet's ActiveTiers; apply truncate / extend rule
//                based on each tier's Plan[0]. Runs LAST in the AI group so
//                all tiers have planned for the frame before chain mutation.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_ActionSet_ChainUpdate : public ck_exp::TProcessor<
	FProcessor_Goap_ActionSet_ChainUpdate,
	FCk_Handle_Goap_ActionSet,
	ck::TReadOnly<FFragment_Goap_ActionSet_Params>,
	ck::TReadOnly<FFragment_Goap_ActionSet_Current>,
	ck::TReadWrite<FFragment_Goap_ActionSet_ActiveTiers>,
	ck::TReadOnly<FFragment_Goap_ActionSet_TierCatalogIndex>,
	CK_IGNORE_PENDING_KILL>
{
public:
	using Group = FGroup_Gameplay_AI;
	using RunAfter = TDepList<FProcessor_Goap_Tier_HandleResult>;

public:
	using TProcessor::TProcessor;

public:
	auto
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionSet_Params& InParams,
		const FFragment_Goap_ActionSet_Current& InCurrent,
		FFragment_Goap_ActionSet_ActiveTiers& InActiveTiers,
		const FFragment_Goap_ActionSet_TierCatalogIndex& InCatalogIndex) const -> void;

private:
	// These helpers mutate private members of FFragment_Goap_Tier_Current.
	// Declared as static members so they inherit the processor's friend access.
	static auto
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Tier& InParentTier,
		TSubclassOf<class UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Tier& InChildTier,
		FFragment_Goap_Tier_Current& InChildCurrent) -> void;

	static auto
	DoTruncateChainFrom(
		TArray<FCk_Handle_Goap_Tier>& InActiveTiers,
		int32 InStartIndex) -> void;
};

// ====================================================================================================================

} // namespace ck
