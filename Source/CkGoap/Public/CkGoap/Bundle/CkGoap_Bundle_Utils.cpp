#include "CkGoap/Bundle/CkGoap_Bundle_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Bundle/CkGoap_Bundle_Fragment.h"
#include "CkGoap/Bundle/CkGoap_Bundle_Record_Internal.h"  // FFragment_RecordOfGoapBundles + utils struct
#include "CkGoap/Tier/CkGoap_Tier_Fragment.h"
#include "CkGoap/CkGoap_Utils.h"  // UCk_Utils_Goap_UE::Find_Bundle (uniqueness check)

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// ====================================================================================================================

auto
	UCk_Utils_Goap_Bundle_UE::
	AddBundle(
		FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_BundleParamsData& InParams)
	-> FCk_Handle_Goap_Bundle
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid Goap root handle when adding bundle"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_BundleTag().IsValid(),
		TEXT("Bundle params has invalid _BundleTag (Goap root [{}])"), InGoap)
	{ return {}; }

	// Diagnostic: bundle-tag uniqueness within root.
	if (auto Existing = UCk_Utils_Goap_UE::Find_Bundle(InGoap, InParams.Get_BundleTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("Bundle with tag [{}] already exists on Goap root [{}]; AddBundle rejected."),
			InParams.Get_BundleTag(), InGoap);
		return {};
	}

	auto BundleEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Bundle>(InGoap);

	BundleEntity.Add<ck::FFragment_Goap_Bundle_Params>(InParams);
	BundleEntity.Add<ck::FFragment_Goap_Bundle_Current>();

	auto& Current = BundleEntity.Get<ck::FFragment_Goap_Bundle_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	BundleEntity.Add<ck::FFragment_Goap_Bundle_ActiveTiers>();
	BundleEntity.Add<ck::FFragment_Goap_Bundle_TierCatalogIndex>();

	// Register the bundle in the root's record.
	ck::goap::internal_root::FRecordOfGoapBundles_Utils::Request_Connect(InGoap, BundleEntity);

	return BundleEntity;
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Bundle_Params>();
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Find_Tier(
		const FCk_Handle_Goap_Bundle& InBundle,
		FGameplayTag InTierTag) -> FCk_Handle_Goap_Tier
{
	if (NOT ck::IsValid(InBundle)) { return {}; }
	if (NOT InTierTag.IsValid()) { return {}; }

	const auto& Index = InBundle.Get<ck::FFragment_Goap_Bundle_TierCatalogIndex>();
	const auto* Found = Index.Get_TagToTier().Find(InTierTag);
	return Found ? *Found : FCk_Handle_Goap_Tier{};
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Get_ActiveTiers(const FCk_Handle_Goap_Bundle& InBundle) -> TArray<FCk_Handle_Goap_Tier>
{
	if (NOT ck::IsValid(InBundle)) { return {}; }
	return InBundle.Get<ck::FFragment_Goap_Bundle_ActiveTiers>().Get_Tiers();
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Get_EnableToggle(const FCk_Handle_Goap_Bundle& InBundle) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InBundle)) { return ECk_EnableDisable::Disable; }
	return InBundle.Get<ck::FFragment_Goap_Bundle_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_Bundle& InBundle) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InBundle)) { return {}; }
	return InBundle.Get<ck::FFragment_Goap_Bundle_Current>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_Bundle& InBundle,
		ECk_EnableDisable InToggle) -> FCk_Handle_Goap_Bundle
{
	CK_ENSURE_IF_NOT(ck::IsValid(InBundle),
		TEXT("Invalid bundle handle in Request_SetEnableToggle"))
	{ return InBundle; }

	auto& Current = InBundle.Get<ck::FFragment_Goap_Bundle_Current>();
	Current._EnableToggle = InToggle;
	return InBundle;
}

auto
	UCk_Utils_Goap_Bundle_UE::
	Request_ResetActiveTiers(FCk_Handle_Goap_Bundle& InBundle) -> FCk_Handle_Goap_Bundle
{
	CK_ENSURE_IF_NOT(ck::IsValid(InBundle),
		TEXT("Invalid bundle handle in Request_ResetActiveTiers"))
	{ return InBundle; }

	// Truncate everything past the root (index 0). The ChainUpdate processor's
	// truncate path normally handles per-tier teardown (signal firing,
	// unsubscribe, etc.); for a direct reset we set a "needs setup" tag and
	// let the chain-update processor pick it up next tick.
	auto& ActiveTiers = InBundle.Get<ck::FFragment_Goap_Bundle_ActiveTiers>();
	if (ActiveTiers._Tiers.Num() > 1)
	{
		ActiveTiers._Tiers.SetNum(1);
		InBundle.AddOrGet<ck::FTag_Goap_Bundle_RequiresChainUpdate>();
	}
	return InBundle;
}

auto
	UCk_Utils_Goap_Bundle_UE::
	BindTo_OnActiveTiersChanged(
		FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Bundle
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Bundle_ActiveTiersChanged,
		InBundle, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InBundle;
}

auto
	UCk_Utils_Goap_Bundle_UE::
	UnbindFrom_OnActiveTiersChanged(
		FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate) -> FCk_Handle_Goap_Bundle
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Bundle_ActiveTiersChanged,
		InBundle, InDelegate);
	return InBundle;
}
