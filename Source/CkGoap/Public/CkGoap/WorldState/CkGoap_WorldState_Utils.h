#pragma once

#include "CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_WorldState_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_WorldState"))
class CKGOAP_API UCk_Utils_Goap_WorldState_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_WorldState_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_WorldState);

public:
// --------------------------------------------------------------------------------------------------------------------
	// Add stamps the WorldState fragments directly on InOwner (the owner IS the WorldState);
	// Create spawns a named child entity instead, for owners hosting more than one. Either
	// handle feeds a planner's _WorldStateSource; lifetime is cascade-bound to InOwner.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Add")
	static FCk_Handle_Goap_WorldState
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		const FCk_Fragment_Goap_WorldState_ParamsData& InParams);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Create WorldState (named child)")
	static FCk_Handle_Goap_WorldState
	Create(
		UPARAM(ref) FCk_Handle& InOwner,
		FGameplayTag InName,
		const FCk_Fragment_Goap_WorldState_ParamsData& InParams);

// --------------------------------------------------------------------------------------------------------------------
	// Keys are auto-registered on first Set. Get returns false for both "key registered,
	// current value false" and "key unregistered" — use Has_Key when the distinction matters.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Set Value",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_WorldState
	Set_Value(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey,
		bool InValue,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Value")
	static bool
	Get_Value(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Has Key")
	static bool
	Has_Key(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Register Key",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_WorldState
	Request_RegisterKey(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

// --------------------------------------------------------------------------------------------------------------------
	// Named override layers shadow the base store on READ only — Set_Value always writes the base.
	// Push / pop / clear fire FTag_Goap_Dirty_WorldState on subscribers ONLY for keys whose effective
	// value changes; re-pushing an existing layer name REPLACES its contents idempotently.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Push Override")
	static FCk_Handle_Goap_WorldState
	Push_Override(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName,
		const TMap<FGameplayTag, bool>& InOverrideValues);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Push Override Single Key")
	static FCk_Handle_Goap_WorldState
	Push_Override_SingleKey(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName,
		FGameplayTag InKey,
		bool InValue);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Pop Override By Name")
	static FCk_Handle_Goap_WorldState
	Pop_Override_ByName(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FName InLayerName);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Clear Overrides")
	static FCk_Handle_Goap_WorldState
	Clear_Overrides(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Override Depth")
	static int32
	Get_OverrideDepth(const FCk_Handle_Goap_WorldState& InWorldState);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Override Layer Names")
	static TArray<FName>
	Get_OverrideLayerNames(const FCk_Handle_Goap_WorldState& InWorldState);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Has Key Override")
	static bool
	Has_KeyOverride(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	// NAME_None when no layer shadows the key (i.e. the read falls through to the base store).
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Top Override Layer For Key")
	static FName
	Get_TopOverrideLayerForKey(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Layer Values")
	static TMap<FGameplayTag, bool>
	Get_LayerValues(const FCk_Handle_Goap_WorldState& InWorldState, FName InLayerName);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Layer Key Count")
	static int32
	Get_LayerKeyCount(const FCk_Handle_Goap_WorldState& InWorldState, FName InLayerName);

	// Bounded ring, oldest first: base Set_Value writes AND override push/pop/clear deltas,
	// each stamped with its mutator and frame number.
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Recent Changes")
	static TArray<FCk_Goap_WorldStateChange>
	Get_RecentChanges(const FCk_Handle_Goap_WorldState& InWorldState);

// --------------------------------------------------------------------------------------------------------------------
	// A subscriber is tagged FTag_Goap_Dirty_WorldState whenever a Set on this WorldState actually
	// changes a value, feeding the AutoReplan OnWorldStateDirty / OnEitherDirty policies. Actions
	// subscribe at activation and unsubscribe at deactivation; external consumers may subscribe too.
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Add Subscriber",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_WorldState
	Request_AddSubscriber(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		UPARAM(ref) FCk_Handle& InSubscriber,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Remove Subscriber",
		meta = (AutoCreateRefTerm = "InDelegate"))
	static FCk_Handle_Goap_WorldState
	Request_RemoveSubscriber(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		UPARAM(ref) FCk_Handle& InSubscriber,
		const FCk_Delegate_Request_OnCompleted& InDelegate);

	// Counts only currently-valid subscribers; stale entries are excluded.
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Subscriber Count")
	static int32
	Get_SubscriberCount(const FCk_Handle_Goap_WorldState& InWorldState);

// --------------------------------------------------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Bind To OnValueChanged")
	static FCk_Handle_Goap_WorldState
	BindTo_OnValueChanged(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		const FCk_Delegate_Goap_WorldState_OnValueChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Unbind From OnValueChanged")
	static FCk_Handle_Goap_WorldState
	UnbindFrom_OnValueChanged(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		const FCk_Delegate_Goap_WorldState_OnValueChanged& InDelegate);

// --------------------------------------------------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Has Feature")
	static bool
	Has(const FCk_Handle& InHandle);

	// Returns the first WorldState child of InHandle, or invalid if none.
	// For owners that host multiple WorldStates, prefer Find_ByName.
	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Find WorldState For Owner")
	static FCk_Handle_Goap_WorldState
	Find(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Find WorldState By Name")
	static FCk_Handle_Goap_WorldState
	Find_ByName(const FCk_Handle& InHandle, FGameplayTag InName);

// --------------------------------------------------------------------------------------------------------------------

private:
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Cast",
		meta = (ExpandEnumAsExecs = "OutResult"))
	static FCk_Handle_Goap_WorldState
	DoCast(
		UPARAM(ref) FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Handle -> WorldState Handle",
		meta = (CompactNodeTitle = "<AsGoapWS>", BlueprintAutocast))
	static FCk_Handle_Goap_WorldState
	DoCastChecked(FCk_Handle InHandle);

	UFUNCTION(BlueprintPure,
		DisplayName = "[Ck] Get Invalid GOAP WorldState Handle",
		Category = "Ck|GOAP|WorldState",
		meta = (CompactNodeTitle = "INVALID_GoapWSHandle", Keywords = "make"))
	static FCk_Handle_Goap_WorldState
	Get_InvalidHandle() { return {}; }

private:
	static auto
	DoAddRequest(
		FCk_Handle_Goap_WorldState& InWorldState,
		const auto& InRequest,
		const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_Goap_WorldState;

	// Lives on this Utils class rather than as a free function so it has friend access to the
	// Subscribers fragment's private _Subscribers.
	static auto
	DoTagSubscribersDirty(FCk_Handle_Goap_WorldState& InWorldState) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
