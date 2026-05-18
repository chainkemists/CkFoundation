#include "CkGoap_WorldState_Utils.h"

#include "CkGoap_WorldState_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// ====================================================================================================================
// RECORD — owner's record of WorldState children created by Create.
// ====================================================================================================================
//
// Defined in this .cpp (not a public Fragment.h) for the same reason
// FFragment_RecordOfGoapPlanners lives in CkGoap_Utils.cpp: exposing
// CkRecord_Fragment.h in a public header would drag FCk_Handle_EntityExtension
// into every CkGoap consumer and break link in dependents that don't list
// CkEntityExtension. Only this translation unit ever touches the record.

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapWorldStates, FCk_Handle_Goap_WorldState);
}

namespace
{
	struct FRecordOfGoapWorldStates_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapWorldStates> {};
}

// ====================================================================================================================
// CREATION
// ====================================================================================================================

namespace
{
	auto
		DoStampWorldStateFragments(
			FCk_Handle& InTargetEntity,
			const FCk_Fragment_Goap_WorldState_ParamsData& /*InParams*/)
		-> void
	{
		// Note: FFragment_Goap_WorldState_Params is currently empty so it isn't
		// stamped — the registry rejects empty structs through the fragment path.
		// When the params struct gains real configuration knobs, re-add the
		// `InTargetEntity.Add<ck::FFragment_Goap_WorldState_Params>(InParams)`
		// stamping here.
		InTargetEntity.Add<ck::FFragment_Goap_WorldState_KeyRegistry>();
		InTargetEntity.Add<ck::FFragment_Goap_WorldState_Values>();
	}
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Create(
		FCk_Handle& InOwner,
		FGameplayTag InName,
		const FCk_Fragment_Goap_WorldState_ParamsData& InParams)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when creating GOAP WorldState"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InName.IsValid(),
		TEXT("Invalid name passed to Create for GOAP WorldState under owner [{}]"),
		InOwner)
	{ return {}; }

	auto WSEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_WorldState>(InOwner);

	UCk_Utils_Handle_UE::Set_DebugName(WSEntity, InName.GetTagName());

	DoStampWorldStateFragments(WSEntity, InParams);

	UCk_Utils_GameplayLabel_UE::Add(WSEntity, InName);

	FRecordOfGoapWorldStates_Utils::AddIfMissing(InOwner,
		ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
	FRecordOfGoapWorldStates_Utils::Request_Connect(InOwner, WSEntity);

	return WSEntity;
}

// ====================================================================================================================
// VALUES
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	Set_Value(
		FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey,
		bool InValue)
	-> FCk_Handle_Goap_WorldState
{
	return DoAddRequest(InWorldState, FCk_Request_Goap_WorldState_SetValue{InKey, InValue});
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_Value(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey)
	-> bool
{
	if (NOT ck::IsValid(InWorldState)) { return false; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_KeyRegistry>()) { return false; }

	const auto& Registry = InWorldState.Get<ck::FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
	const auto Key = Registry.Find(InKey);
	if (Key == ck::goap::InvalidGoapKey) { return false; }

	const auto& Values = InWorldState.Get<ck::FFragment_Goap_WorldState_Values>().Get_Values();
	return Values.Get(Key);
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Has_Key(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey)
	-> bool
{
	if (NOT ck::IsValid(InWorldState)) { return false; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_KeyRegistry>()) { return false; }
	const auto& Registry = InWorldState.Get<ck::FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
	return Registry.Find(InKey) != ck::goap::InvalidGoapKey;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Request_RegisterKey(
		FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey)
	-> FCk_Handle_Goap_WorldState
{
	return DoAddRequest(InWorldState, FCk_Request_Goap_WorldState_RegisterKey{InKey});
}

// ====================================================================================================================
// SIGNAL BINDING
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	BindTo_OnValueChanged(
		FCk_Handle_Goap_WorldState& InWorldState,
		const FCk_Delegate_Goap_WorldState_OnValueChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior)
	-> FCk_Handle_Goap_WorldState
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoapWorldStateValueChanged, InWorldState, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	UnbindFrom_OnValueChanged(
		FCk_Handle_Goap_WorldState& InWorldState,
		const FCk_Delegate_Goap_WorldState_OnValueChanged& InDelegate)
	-> FCk_Handle_Goap_WorldState
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoapWorldStateValueChanged, InWorldState, InDelegate);
	return InWorldState;
}

// ====================================================================================================================
// QUERY
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	Has(const FCk_Handle& InHandle)
	-> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_WorldState_Values>();
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Find(const FCk_Handle& InHandle)
	-> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InHandle))
	{ return {}; }

	if (InHandle.Has<ck::FFragment_Goap_WorldState_Values>())
	{ return CastChecked(InHandle); }

	if (NOT FRecordOfGoapWorldStates_Utils::Has(InHandle))
	{ return {}; }

	const auto Entries = FRecordOfGoapWorldStates_Utils::Get_ValidEntries(InHandle);
	if (Entries.IsEmpty())
	{ return {}; }

	return Entries[0];
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Find_ByName(const FCk_Handle& InHandle, FGameplayTag InName)
	-> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InHandle))
	{ return {}; }

	if (NOT FRecordOfGoapWorldStates_Utils::Has(InHandle))
	{ return {}; }

	return FRecordOfGoapWorldStates_Utils::Get_ValidEntry_ByTag(InHandle, InName);
}

// ====================================================================================================================
// CAST
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	DoCast(FCk_Handle& InHandle, ECk_SucceededFailed& OutResult)
	-> FCk_Handle_Goap_WorldState
{
	if (Has(InHandle))
	{
		OutResult = ECk_SucceededFailed::Succeeded;
		return Cast(InHandle);
	}
	OutResult = ECk_SucceededFailed::Failed;
	return {};
}

auto
	UCk_Utils_Goap_WorldState_UE::
	DoCastChecked(FCk_Handle InHandle)
	-> FCk_Handle_Goap_WorldState
{
	return CastChecked(InHandle);
}

// ====================================================================================================================
// INTERNALS
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	DoAddRequest(FCk_Handle_Goap_WorldState& InWorldState, const auto& InRequest)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InWorldState), TEXT("Invalid GOAP WorldState handle when adding request"))
	{ return InWorldState; }

	auto& Requests = InWorldState.AddOrGet<ck::FFragment_Goap_WorldState_Requests>();
	Requests._Requests.Add(InRequest);
	return InWorldState;
}

// ====================================================================================================================
