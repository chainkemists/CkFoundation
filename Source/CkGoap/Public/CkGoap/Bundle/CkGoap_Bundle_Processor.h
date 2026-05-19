#pragma once

#include "CkGoap/Bundle/CkGoap_Bundle_Fragment.h"
#include "CkGoap/Tier/CkGoap_Tier_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// CHAIN UPDATE — Walk each bundle's ActiveTiers; apply truncate / extend rule
//                based on each tier's Plan[0]. Runs LAST in the AI group so
//                all tiers have planned for the frame before chain mutation.
// ====================================================================================================================

class CKGOAP_API FProcessor_Goap_Bundle_ChainUpdate : public ck_exp::TProcessor<
	FProcessor_Goap_Bundle_ChainUpdate,
	FCk_Handle_Goap_Bundle,
	ck::TReadOnly<FFragment_Goap_Bundle_Params>,
	ck::TReadOnly<FFragment_Goap_Bundle_Current>,
	ck::TReadWrite<FFragment_Goap_Bundle_ActiveTiers>,
	ck::TReadOnly<FFragment_Goap_Bundle_TierCatalogIndex>,
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
		const FFragment_Goap_Bundle_Params& InParams,
		const FFragment_Goap_Bundle_Current& InCurrent,
		FFragment_Goap_Bundle_ActiveTiers& InActiveTiers,
		const FFragment_Goap_Bundle_TierCatalogIndex& InCatalogIndex) const -> void;

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
