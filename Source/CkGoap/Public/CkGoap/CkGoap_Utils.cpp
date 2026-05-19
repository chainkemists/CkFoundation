#include "CkGoap_Utils.h"

#include "CkGoap/CkGoap_Fragment.h"
#include "CkGoap/Bundle/CkGoap_Bundle_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// ====================================================================================================================
// RECORD — root's record of bundle child entities.
//
// Defined in this .cpp (not the public Fragment.h) on purpose: declaring the
// record in a public header would drag CkRecord_Fragment.h into every CkGoap
// consumer, which forces a transitive reference to FCk_Handle_EntityExtension
// and breaks link in dependents that don't list CkEntityExtension.
//
// The presence of this fragment on an entity is also the "is a GOAP root"
// marker — Has(InHandle) checks for it.
// ====================================================================================================================

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapBundles, FCk_Handle_Goap_Bundle);
}

namespace ck::goap::internal_root
{
	struct FRecordOfGoapBundles_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapBundles> {};
}

// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Add(
		FCk_Handle& InOwner,
		const FCk_Fragment_Goap_RootParamsData& /*InParams*/)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when adding GOAP root"))
	{ return {}; }

	// Spawn a typesafe child entity as the Goap root container. Owner-cascade
	// destroy reaches it via the parent chain. RootParamsData is currently
	// empty (reserved for future global tuning); we don't stamp it as an ECS
	// fragment — the record-of-bundles fragment IS the "Goap root" marker.
	auto GoapEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap>(InOwner);
	ck::goap::internal_root::FRecordOfGoapBundles_Utils::AddIfMissing(GoapEntity);

	return GoapEntity;
}

auto
	UCk_Utils_Goap_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_RecordOfGoapBundles>();
}

auto
	UCk_Utils_Goap_UE::
	Cast(const FCk_Handle& InHandle) -> FCk_Handle_Goap
{
	return UCk_Utils_Goap_UE::CastChecked(InHandle);
}

auto
	UCk_Utils_Goap_UE::
	Find_Bundle(
		const FCk_Handle_Goap& InGoap,
		FGameplayTag InBundleTag) -> FCk_Handle_Goap_Bundle
{
	auto Result = FCk_Handle_Goap_Bundle{};

	if (NOT ck::IsValid(InGoap)) { return Result; }
	if (NOT InBundleTag.IsValid()) { return Result; }

	auto MutableGoap = InGoap;
	ck::goap::internal_root::FRecordOfGoapBundles_Utils::ForEach_ValidEntry(
		MutableGoap,
		[&](FCk_Handle_Goap_Bundle InBundle)
		{
			if (NOT ck::IsValid(InBundle)) { return; }
			const auto& Params = InBundle.Get<ck::FFragment_Goap_Bundle_Params>();
			if (Params.Get_BundleTag() == InBundleTag)
			{
				Result = InBundle;
			}
		});

	return Result;
}
