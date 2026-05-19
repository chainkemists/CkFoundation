#include "CkGoap/Bundle/CkGoap_Bundle_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Tier/CkGoap_Tier_Fragment.h"
#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Bundle_ChainUpdate);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// HELPERS — static members of FProcessor_Goap_Bundle_ChainUpdate so they
// inherit the processor's friend access to FFragment_Goap_Tier_Current.
// ====================================================================================================================

auto
	FProcessor_Goap_Bundle_ChainUpdate::
	DoInjectGoalSynchronous(
		FCk_Handle_Goap_Tier& InParentTier,
		TSubclassOf<UCk_GoapAction_EntityScript> InParentActionClass,
		FCk_Handle_Goap_Tier& InChildTier,
		FFragment_Goap_Tier_Current& InChildCurrent) -> void
{
	InChildCurrent._Goal.Reset();
	InChildCurrent._InvalidGoal.Reset();

	const auto& ParentActions = InParentTier.Get<FFragment_Goap_Tier_Actions>().Get_ActionDefs();
	const goap::FActionDef* ParentDef = nullptr;
	for (const auto& D : ParentActions)
	{
		if (D.ActionClass == InParentActionClass) { ParentDef = &D; break; }
	}
	if (ParentDef == nullptr) { return; }

	const auto ParentWS = InParentTier.Get<FFragment_Goap_Tier_Current>().Get_WorldStateSource_Resolved();
	const auto ChildWS  = InChildCurrent.Get_WorldStateSource_Resolved();
	if (NOT ck::IsValid(ParentWS) || NOT ck::IsValid(ChildWS)) { return; }

	const auto& ParentRegistry = const_cast<FCk_Handle_Goap_WorldState&>(ParentWS)
		.Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
	const auto& ChildRegistry = const_cast<FCk_Handle_Goap_WorldState&>(ChildWS)
		.Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();

	for (const auto& Effect : ParentDef->Effects)
	{
		// Round-trip: parent key → raw tag → child key.
		const auto RawTag = ParentRegistry.GetTag(Effect.Key);
		if (NOT RawTag.IsValid()) { continue; }

		const auto ChildKey = ChildRegistry.Find(RawTag);
		if (ChildKey == goap::InvalidGoapKey)
		{
			InChildCurrent._InvalidGoal.Add(
				FCk_GoapWS_Condition_Authored{RawTag, Effect.Value});
		}
		else
		{
			InChildCurrent._Goal.Add(goap::FWorldStateCondition{ChildKey, Effect.Value});
		}
	}
}

auto
	FProcessor_Goap_Bundle_ChainUpdate::
	DoTruncateChainFrom(
		TArray<FCk_Handle_Goap_Tier>& InActiveTiers,
		int32 InStartIndex) -> void
{
	for (auto i = InActiveTiers.Num() - 1; i >= InStartIndex; --i)
	{
		auto& Tier = InActiveTiers[i];
		auto& Current = Tier.Get<FFragment_Goap_Tier_Current>();

		// Unsubscribe the tier from its resolved WS before tearing down state.
		if (ck::IsValid(Current._WorldStateSource_Resolved))
		{
			auto WS = Current._WorldStateSource_Resolved;
			::UCk_Utils_Goap_WorldState_UE::Request_RemoveSubscriber(WS, Tier);
		}

		Current._Goal.Reset();
		Current._InvalidGoal.Reset();
		Current._ActiveParentAction = nullptr;
		Current._WorldStateSource_Resolved = {};
		Current._Plan.Reset();
		Current._PlanStatus = ECk_GoapPlanStatus::Idle;

		UUtils_Signal_OnGoap_Tier_Deactivated::Broadcast(
			Tier, ck::MakePayload(Tier, FCk_Goap_Payload_OnTierDeactivated{}));

		InActiveTiers.RemoveAt(i);
	}
}

// ====================================================================================================================

auto
	FProcessor_Goap_Bundle_ChainUpdate::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Bundle_Params& InParams,
		const FFragment_Goap_Bundle_Current& InCurrent,
		FFragment_Goap_Bundle_ActiveTiers& InActiveTiers,
		const FFragment_Goap_Bundle_TierCatalogIndex& InCatalogIndex) const -> void
{
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	auto& Tiers = InActiveTiers._Tiers;
	if (Tiers.IsEmpty()) { return; }

	const auto& Catalog = InCatalogIndex.Get_TagToTier();
	const auto OldChainSnapshot = Tiers;
	auto bChainChanged = false;

	auto i = int32{0};
	while (i < Tiers.Num())
	{
		auto& CurrTier = Tiers[i];
		auto& CurrCurrent = CurrTier.Get<FFragment_Goap_Tier_Current>();

		if (CurrCurrent._PlanStatus != ECk_GoapPlanStatus::PlanFound)
		{
			// Freshly-appended / planning / failed — don't walk past.
			break;
		}

		const auto& Plan = CurrCurrent._Plan;
		if (Plan.IsEmpty())
		{
			// Goal already satisfied / no actions. Truncate any children.
			if (i + 1 < Tiers.Num())
			{
				DoTruncateChainFrom(Tiers, i + 1);
				bChainChanged = true;
			}
			break;
		}

		const auto NextActionClass = Plan[0];
		const auto* CDO = NextActionClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO)) { break; }
		const auto NextActionTag = CDO->Get_ActionTag();

		const auto* MatchingTierPtr = Catalog.Find(NextActionTag);
		const auto MatchingTier = MatchingTierPtr ? *MatchingTierPtr : FCk_Handle_Goap_Tier{};

		if (i + 1 < Tiers.Num())
		{
			// Already have a child. Does it still match?
			const auto& ExistingChild = Tiers[i + 1];
			const auto ExistingChildTag = ExistingChild
				.Get<FFragment_Goap_Tier_Params>().Get_TierTag();
			if (ExistingChildTag == NextActionTag)
			{
				++i;
				continue;
			}
			// Mismatch — truncate, then fall through.
			DoTruncateChainFrom(Tiers, i + 1);
			bChainChanged = true;
		}

		// At the leaf. Try to append.
		if (NOT ck::IsValid(MatchingTier)) { break; }

		// Capture mutable copy of the matching tier handle.
		auto MatchingTierMutable = MatchingTier;
		auto& MatchingCurrent = MatchingTierMutable.Get<FFragment_Goap_Tier_Current>();

		// Resolve WS source: override if set, else inherit parent's resolved.
		const auto& MatchingParams = MatchingTierMutable.Get<FFragment_Goap_Tier_Params>();
		if (ck::IsValid(MatchingParams.Get_WorldStateSource_Override()))
		{
			MatchingCurrent._WorldStateSource_Resolved = MatchingParams.Get_WorldStateSource_Override();
		}
		else
		{
			MatchingCurrent._WorldStateSource_Resolved = CurrCurrent._WorldStateSource_Resolved;
		}

		// Inject goal synchronously from parent action's Effects.
		DoInjectGoalSynchronous(CurrTier, NextActionClass, MatchingTierMutable, MatchingCurrent);

		MatchingCurrent._ActiveParentAction = NextActionClass;

		// Subscribe the child tier to its resolved WS for dirty propagation.
		if (ck::IsValid(MatchingCurrent._WorldStateSource_Resolved))
		{
			auto WS = MatchingCurrent._WorldStateSource_Resolved;
			::UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, MatchingTierMutable);
		}

		// Append to chain and mark for re-setup + first plan.
		Tiers.Add(MatchingTierMutable);
		MatchingTierMutable.AddOrGet<FTag_Goap_Tier_RequiresSetup>();
		MatchingTierMutable.AddOrGet<FTag_Goap_Tier_RequiresInitialPlan>();

		bChainChanged = true;

		UUtils_Signal_OnGoap_Tier_Activated::Broadcast(
			MatchingTierMutable, ck::MakePayload(MatchingTierMutable, FCk_Goap_Payload_OnTierActivated{}));

		// Defer newly-appended tier's first plan to next frame.
		break;
	}

	if (bChainChanged)
	{
		UUtils_Signal_OnGoap_Bundle_ActiveTiersChanged::Broadcast(
			InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnActiveTiersChanged{OldChainSnapshot}));
	}
}

// ====================================================================================================================

} // namespace ck
