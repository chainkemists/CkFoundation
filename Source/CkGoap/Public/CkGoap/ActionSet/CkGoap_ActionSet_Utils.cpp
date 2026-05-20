#include "CkGoap/ActionSet/CkGoap_ActionSet_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Record_Internal.h"  // FFragment_RecordOfGoapActionSets + utils struct
#include "CkGoap/Tier/CkGoap_Tier_Fragment.h"
#include "CkGoap/CkGoap_Utils.h"  // UCk_Utils_Goap_UE::Find_ActionSet (uniqueness check)

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================

auto
	UCk_Utils_Goap_ActionSet_UE::
	AddActionSet(
		FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_ActionSetParamsData& InParams)
	-> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid Goap root handle when adding ActionSet"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_ActionSetTag().IsValid(),
		TEXT("ActionSet params has invalid _ActionSetTag (Goap root [{}])"), InGoap)
	{ return {}; }

	// Diagnostic: ActionSet-tag uniqueness within root.
	if (auto Existing = UCk_Utils_Goap_UE::Find_ActionSet(InGoap, InParams.Get_ActionSetTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("ActionSet with tag [{}] already exists on Goap root [{}]; AddActionSet rejected."),
			InParams.Get_ActionSetTag(), InGoap);
		return {};
	}

	auto ActionSetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_ActionSet>(InGoap);

	// Records of ActionSets require GameplayLabels — label the ActionSet with its
	// declared tag. Same for tiers in AddTier.
	UCk_Utils_GameplayLabel_UE::Add(ActionSetEntity, InParams.Get_ActionSetTag());

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Params>(InParams);
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Current>();

	auto& Current = ActionSetEntity.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_ActiveTiers>();
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_TierCatalogIndex>();
	ActionSetEntity.AddOrGet<ck::FTag_Goap_ActionSet_RequiresSetup>();

	// Register the ActionSet in the root's record.
	ck::goap::internal_root::FRecordOfGoapActionSets_Utils::Request_Connect(InGoap, ActionSetEntity);

	return ActionSetEntity;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_ActionSet_Params>();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Find_Tier(
		const FCk_Handle_Goap_ActionSet& InActionSet,
		FGameplayTag InTierTag) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	if (NOT InTierTag.IsValid()) { return {}; }

	const auto& Index = InActionSet.Get<ck::FFragment_Goap_ActionSet_TierCatalogIndex>();
	const auto* Found = Index.Get_TagToTier().Find(InTierTag);
	return Found ? *Found : FCk_Handle_Goap_Tier{};
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_ActiveTiers(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_Handle_Goap_Tier>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveTiers>().Get_Tiers();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_EnableToggle(const FCk_Handle_Goap_ActionSet& InActionSet) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InActionSet)) { return ECk_EnableDisable::Disable; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_ActionSet& InActionSet,
		ECk_EnableDisable InToggle) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_SetEnableToggle"))
	{ return InActionSet; }

	auto& Current = InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InToggle;
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_ResetActiveTiers(FCk_Handle_Goap_ActionSet& InActionSet) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_ResetActiveTiers"))
	{ return InActionSet; }

	// Truncate everything past the root (index 0). The ChainUpdate processor's
	// truncate path normally handles per-tier teardown (signal firing,
	// unsubscribe, etc.); for a direct reset we set a "needs setup" tag and
	// let the chain-update processor pick it up next tick.
	auto& ActiveTiers = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveTiers>();
	if (ActiveTiers._Tiers.Num() > 1)
	{
		ActiveTiers._Tiers.SetNum(1);
		InActionSet.AddOrGet<ck::FTag_Goap_ActionSet_RequiresChainUpdate>();
	}
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	BindTo_OnActiveTiersChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveTiersChanged,
		InActionSet, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	UnbindFrom_OnActiveTiersChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveTiersChanged,
		InActionSet, InDelegate);
	return InActionSet;
}
