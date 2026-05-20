#include "CkGoap_Utils.h"

#include "CkGoap/CkGoap_Fragment.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Record_Internal.h"  // FFragment_RecordOfGoapActionSets + utils struct

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

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
	// fragment — the record-of-actionsets fragment IS the "Goap root" marker.
	auto GoapEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap>(InOwner);
	ck::goap::internal_root::FRecordOfGoapActionSets_Utils::AddIfMissing(GoapEntity);

	return GoapEntity;
}

auto
	UCk_Utils_Goap_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_RecordOfGoapActionSets>();
}

auto
	UCk_Utils_Goap_UE::
	Find_ActionSet(
		const FCk_Handle_Goap& InGoap,
		FGameplayTag InActionSetTag) -> FCk_Handle_Goap_ActionSet
{
	auto Result = FCk_Handle_Goap_ActionSet{};

	if (NOT ck::IsValid(InGoap)) { return Result; }
	if (NOT InActionSetTag.IsValid()) { return Result; }

	auto MutableGoap = InGoap;
	ck::goap::internal_root::FRecordOfGoapActionSets_Utils::ForEach_ValidEntry(
		MutableGoap,
		[&](FCk_Handle_Goap_ActionSet InActionSet)
		{
			if (NOT ck::IsValid(InActionSet)) { return; }
			const auto& Params = InActionSet.Get<ck::FFragment_Goap_ActionSet_Params>();
			if (Params.Get_ActionSetTag() == InActionSetTag)
			{
				Result = InActionSet;
			}
		});

	return Result;
}
