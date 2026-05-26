#include "CkGoap_WorldState_Utils.h"

#include "CkGoap_WorldState_Fragment.h"

#include "CkGoap/CkGoap_Fragment.h"      // FTag_Goap_Dirty_WorldState

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
		InTargetEntity.Add<ck::FFragment_Goap_WorldState_OverrideStack>();
		InTargetEntity.Add<ck::FFragment_Goap_WorldState_Subscribers>();
	}
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Add(
		FCk_Handle& InOwner,
		const FCk_Fragment_Goap_WorldState_ParamsData& InParams)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when adding GOAP WorldState"))
	{ return {}; }

	CK_ENSURE_IF_NOT(NOT InOwner.Has<ck::FFragment_Goap_WorldState_Values>(),
		TEXT("Owner [{}] already has a GOAP WorldState — call Find or use Create for a named child instead"),
		InOwner)
	{ return CastChecked(InOwner); }

	DoStampWorldStateFragments(InOwner, InParams);

	return CastChecked(InOwner);
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

	// Override-stack read path: walk top-down, return first match. Layers
	// store raw FGameplayTag keys and resolve lazily at read time so the
	// override-push API doesn't need to be ordered against key registration.
	if (InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>())
	{
		const auto& Stack = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>();
		const auto& Layers = Stack.Get_Layers();
		for (auto i = Layers.Num() - 1; i >= 0; --i)
		{
			if (const auto* V = Layers[i].Values.Find(InKey))
			{
				return *V;
			}
		}
	}

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
// OVERRIDE STACK
// ====================================================================================================================

namespace
{
	// Compute the effective Get_Value(WS, Tag) without going through the public
	// API — walks the override stack top-down, falls through to the base store.
	// Returns the effective bool for the supplied tag.
	auto
		DoGetEffectiveValue(
			const FCk_Handle_Goap_WorldState& InWS,
			FGameplayTag InTag)
		-> bool
	{
		const auto& Stack = InWS.Get<ck::FFragment_Goap_WorldState_OverrideStack>();
		const auto& Layers = Stack.Get_Layers();
		for (auto i = Layers.Num() - 1; i >= 0; --i)
		{
			if (const auto* V = Layers[i].Values.Find(InTag)) { return *V; }
		}

		const auto& Registry = InWS.Get<ck::FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
		const auto Key = Registry.Find(InTag);
		if (Key == ck::goap::InvalidGoapKey) { return false; }
		const auto& Values = InWS.Get<ck::FFragment_Goap_WorldState_Values>().Get_Values();
		return Values.Get(Key);
	}

	// Snapshot the effective value for every tag in `InTags` (typically the
	// keyset of the layer being pushed / popped). Returned in a tag-keyed map.
	auto
		DoSnapshotEffective(
			const FCk_Handle_Goap_WorldState& InWS,
			const TArray<FGameplayTag>& InTags)
		-> TMap<FGameplayTag, bool>
	{
		auto Out = TMap<FGameplayTag, bool>{};
		Out.Reserve(InTags.Num());
		for (const auto& Tag : InTags)
		{
			Out.Add(Tag, DoGetEffectiveValue(InWS, Tag));
		}
		return Out;
	}

}

auto
	UCk_Utils_Goap_WorldState_UE::
	Push_Override(
		FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName,
		const TMap<FGameplayTag, bool>& InOverrideValues)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InWorldState),
		TEXT("Invalid WorldState handle when pushing override layer [{}]"), InLayerName)
	{ return InWorldState; }

	CK_ENSURE_IF_NOT(NOT InLayerName.IsNone(),
		TEXT("Invalid (None) layer name when pushing override on WorldState [{}]"), InWorldState)
	{ return InWorldState; }

	auto& Stack = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>();

	// Build the union of keys we may touch: keys in the new layer plus keys
	// in the layer being REPLACED (if any) — the latter matters because keys
	// dropped during replacement may flip their effective view back to base.
	auto AffectedTags = TArray<FGameplayTag>{};
	AffectedTags.Reserve(InOverrideValues.Num());
	for (const auto& Kv : InOverrideValues) { AffectedTags.AddUnique(Kv.Key); }

	const auto ExistingIdx = Stack._Layers.IndexOfByPredicate(
		[&](const ck::FFragment_Goap_WorldState_OverrideStack::FLayer& InLayer)
		{ return InLayer.Name == InLayerName; });

	if (ExistingIdx != INDEX_NONE)
	{
		for (const auto& Kv : Stack._Layers[ExistingIdx].Values) { AffectedTags.AddUnique(Kv.Key); }
	}

	const auto Before = DoSnapshotEffective(InWorldState, AffectedTags);

	if (ExistingIdx != INDEX_NONE)
	{
		// Replace contents idempotently.
		Stack._Layers[ExistingIdx].Values = InOverrideValues;
	}
	else
	{
		Stack._Layers.Add({InLayerName, InOverrideValues});
	}

	const auto After = DoSnapshotEffective(InWorldState, AffectedTags);

	auto AnyEffectiveChange = false;
	for (const auto& Tag : AffectedTags)
	{
		if (Before.FindRef(Tag) != After.FindRef(Tag)) { AnyEffectiveChange = true; break; }
	}

	if (AnyEffectiveChange)
	{
		DoTagSubscribersDirty(InWorldState);
	}

	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Push_Override_SingleKey(
		FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName,
		FGameplayTag InKey,
		bool InValue)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InWorldState),
		TEXT("Invalid WorldState handle when pushing single-key override [{}]={}"), InKey, InValue)
	{ return InWorldState; }

	CK_ENSURE_IF_NOT(NOT InLayerName.IsNone(),
		TEXT("Invalid (None) layer name when pushing single-key override on WorldState [{}]"), InWorldState)
	{ return InWorldState; }

	CK_ENSURE_IF_NOT(InKey.IsValid(),
		TEXT("Invalid key when pushing single-key override on WorldState [{}]"), InWorldState)
	{ return InWorldState; }

	auto& Stack = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>();

	const auto BeforeEff = DoGetEffectiveValue(InWorldState, InKey);

	const auto ExistingIdx = Stack._Layers.IndexOfByPredicate(
		[&](const ck::FFragment_Goap_WorldState_OverrideStack::FLayer& InLayer)
		{ return InLayer.Name == InLayerName; });

	if (ExistingIdx != INDEX_NONE)
	{
		Stack._Layers[ExistingIdx].Values.FindOrAdd(InKey) = InValue;
	}
	else
	{
		auto NewLayer = ck::FFragment_Goap_WorldState_OverrideStack::FLayer{};
		NewLayer.Name = InLayerName;
		NewLayer.Values.Add(InKey, InValue);
		Stack._Layers.Add(MoveTemp(NewLayer));
	}

	const auto AfterEff = DoGetEffectiveValue(InWorldState, InKey);
	if (BeforeEff != AfterEff)
	{
		DoTagSubscribersDirty(InWorldState);
	}

	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Pop_Override_ByName(
		FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName)
	-> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InWorldState)) { return InWorldState; }

	auto& Stack = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>();
	const auto ExistingIdx = Stack._Layers.IndexOfByPredicate(
		[&](const ck::FFragment_Goap_WorldState_OverrideStack::FLayer& InLayer)
		{ return InLayer.Name == InLayerName; });

	if (ExistingIdx == INDEX_NONE) { return InWorldState; }

	auto AffectedTags = TArray<FGameplayTag>{};
	AffectedTags.Reserve(Stack._Layers[ExistingIdx].Values.Num());
	for (const auto& Kv : Stack._Layers[ExistingIdx].Values) { AffectedTags.AddUnique(Kv.Key); }

	const auto Before = DoSnapshotEffective(InWorldState, AffectedTags);

	Stack._Layers.RemoveAt(ExistingIdx);

	const auto After = DoSnapshotEffective(InWorldState, AffectedTags);

	auto AnyEffectiveChange = false;
	for (const auto& Tag : AffectedTags)
	{
		if (Before.FindRef(Tag) != After.FindRef(Tag)) { AnyEffectiveChange = true; break; }
	}

	if (AnyEffectiveChange)
	{
		DoTagSubscribersDirty(InWorldState);
	}

	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Clear_Overrides(
		FCk_Handle_Goap_WorldState& InWorldState)
	-> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InWorldState)) { return InWorldState; }

	auto& Stack = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>();
	if (Stack._Layers.IsEmpty()) { return InWorldState; }

	// Union of every key across every layer.
	auto AffectedTags = TArray<FGameplayTag>{};
	for (const auto& Layer : Stack._Layers)
	{
		for (const auto& Kv : Layer.Values) { AffectedTags.AddUnique(Kv.Key); }
	}

	const auto Before = DoSnapshotEffective(InWorldState, AffectedTags);

	Stack._Layers.Reset();

	const auto After = DoSnapshotEffective(InWorldState, AffectedTags);

	auto AnyEffectiveChange = false;
	for (const auto& Tag : AffectedTags)
	{
		if (Before.FindRef(Tag) != After.FindRef(Tag)) { AnyEffectiveChange = true; break; }
	}

	if (AnyEffectiveChange)
	{
		DoTagSubscribersDirty(InWorldState);
	}

	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_OverrideDepth(const FCk_Handle_Goap_WorldState& InWorldState)
	-> int32
{
	if (NOT ck::IsValid(InWorldState)) { return 0; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return 0; }
	return InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers().Num();
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_OverrideLayerNames(const FCk_Handle_Goap_WorldState& InWorldState)
	-> TArray<FName>
{
	auto Out = TArray<FName>{};
	if (NOT ck::IsValid(InWorldState)) { return Out; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return Out; }

	const auto& Layers = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers();
	Out.Reserve(Layers.Num());
	for (const auto& Layer : Layers) { Out.Add(Layer.Name); }
	return Out;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Has_KeyOverride(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey)
	-> bool
{
	if (NOT ck::IsValid(InWorldState)) { return false; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return false; }

	const auto& Layers = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers();
	for (const auto& Layer : Layers)
	{
		if (Layer.Values.Contains(InKey)) { return true; }
	}
	return false;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_TopOverrideLayerForKey(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey)
	-> FName
{
	if (NOT ck::IsValid(InWorldState)) { return NAME_None; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return NAME_None; }

	// Walk top-down: most recently pushed layer wins (matches Get_Value semantics).
	const auto& Layers = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers();
	for (auto i = Layers.Num() - 1; i >= 0; --i)
	{
		if (Layers[i].Values.Contains(InKey)) { return Layers[i].Name; }
	}
	return NAME_None;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_LayerValues(const FCk_Handle_Goap_WorldState& InWorldState, FName InLayerName)
	-> TMap<FGameplayTag, bool>
{
	if (NOT ck::IsValid(InWorldState)) { return {}; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return {}; }

	const auto& Layers = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers();
	for (const auto& Layer : Layers)
	{
		if (Layer.Name == InLayerName) { return Layer.Values; }
	}
	return {};
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Get_LayerKeyCount(const FCk_Handle_Goap_WorldState& InWorldState, FName InLayerName)
	-> int32
{
	if (NOT ck::IsValid(InWorldState)) { return 0; }
	if (NOT InWorldState.Has<ck::FFragment_Goap_WorldState_OverrideStack>()) { return 0; }

	const auto& Layers = InWorldState.Get<ck::FFragment_Goap_WorldState_OverrideStack>().Get_Layers();
	for (const auto& Layer : Layers)
	{
		if (Layer.Name == InLayerName) { return Layer.Values.Num(); }
	}
	return 0;
}

// ====================================================================================================================
// SUBSCRIBERS
// ====================================================================================================================

auto
	UCk_Utils_Goap_WorldState_UE::
	Request_AddSubscriber(
		FCk_Handle_Goap_WorldState& InWorldState,
		FCk_Handle& InSubscriber)
	-> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InWorldState),
		TEXT("Invalid WorldState handle when adding subscriber"))
	{ return InWorldState; }
	CK_ENSURE_IF_NOT(ck::IsValid(InSubscriber),
		TEXT("Invalid subscriber handle for WorldState [{}]"), InWorldState)
	{ return InWorldState; }

	auto& Subscribers = InWorldState.Get<ck::FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.AddUnique(InSubscriber);
	return InWorldState;
}

auto
	UCk_Utils_Goap_WorldState_UE::
	Request_RemoveSubscriber(
		FCk_Handle_Goap_WorldState& InWorldState,
		FCk_Handle& InSubscriber)
	-> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InWorldState)) { return InWorldState; }
	auto& Subscribers = InWorldState.Get<ck::FFragment_Goap_WorldState_Subscribers>();
	Subscribers._Subscribers.RemoveSwap(InSubscriber);
	return InWorldState;
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

auto
	UCk_Utils_Goap_WorldState_UE::
	DoTagSubscribersDirty(FCk_Handle_Goap_WorldState& InWorldState)
	-> void
{
	auto& Subscribers = InWorldState.Get<ck::FFragment_Goap_WorldState_Subscribers>();
	for (auto Index = Subscribers._Subscribers.Num() - 1; Index >= 0; --Index)
	{
		auto& Subscriber = Subscribers._Subscribers[Index];
		if (NOT ck::IsValid(Subscriber))
		{
			Subscribers._Subscribers.RemoveAtSwap(Index);
			continue;
		}
		Subscriber.AddOrGet<ck::FTag_Goap_Dirty_WorldState>();
	}
}

// ====================================================================================================================
