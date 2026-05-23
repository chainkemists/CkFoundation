#pragma once

#include "CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_WorldState_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_WorldState"))
class CKGOAP_API UCk_Utils_Goap_WorldState_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_WorldState_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_WorldState);

public:
	// ================================================================================================================
	// CONSTRUCTION
	// ================================================================================================================
	//
	// Add — stamps the GOAP WorldState fragments directly on InOwner. The owner
	//       IS the WorldState; cast it with the typesafe accessor when handing
	//       it to a planner via FCk_Fragment_Goap_PlannerParamsData::_WorldStateSource.
	//       Use this when only one WorldState is needed on the owner and you'd
	//       rather not pay for a child entity.
	//
	// Create — spawns a new named child entity under InOwner that hosts a shared
	//          GOAP WorldState. The caller hands this handle to one or more
	//          GOAP planners via FCk_Fragment_Goap_ParamsData::_WorldStateSource.
	//          Reads, writes, and replan-trigger subscriptions on the planners
	//          all route through the resulting WorldState entity.
	//
	// Lifetime is cascade-bound to InOwner — destroying the owner destroys the
	// WorldState (via the owner's RecordOfGoapWorldStates for Create, or via
	// the owner's own destroy for Add).

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

	// ================================================================================================================
	// VALUES
	// ================================================================================================================
	//
	// Keys are auto-registered on first Set. Get returns false for both
	// "key registered, current value false" and "key unregistered" — use
	// Has_Key when the distinction matters.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Set Value")
	static FCk_Handle_Goap_WorldState
	Set_Value(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey,
		bool InValue);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Get Value")
	static bool
	Get_Value(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	UFUNCTION(BlueprintPure, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Has Key")
	static bool
	Has_Key(const FCk_Handle_Goap_WorldState& InWorldState, FGameplayTag InKey);

	// Force-registers a key without setting its value. Returns the WorldState
	// handle for chaining. Useful when callers want a key to occupy a stable
	// slot before any Set is performed.
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Register Key")
	static FCk_Handle_Goap_WorldState
	Request_RegisterKey(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		FGameplayTag InKey);

	// ================================================================================================================
	// OVERRIDE STACK
	// ================================================================================================================
	//
	// Named override layers shadow the base store on read; writes via Set_Value
	// always go to the base. Push / pop / clear fire FTag_Goap_Dirty_WorldState
	// on subscribers ONLY for keys whose effective value changes — re-pushing
	// the same values is a quiet no-op. A* seeds a flattened snapshot at plan
	// time so the search inner loop never walks the stack.
	//
	// Layers are NAMED. Re-pushing a layer with the same name REPLACES its
	// contents idempotently. The debugger uses a fixed "DebugUI" layer;
	// AI deliberation scopes typically pick ad-hoc names.

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

	// ================================================================================================================
	// SUBSCRIBERS
	// ================================================================================================================
	//
	// Registering an entity as a subscriber on a WorldState causes that entity
	// to be tagged with FTag_Goap_Dirty_WorldState whenever a Set request on
	// this WorldState actually changes a key's value — which feeds per-Action
	// AutoReplan throttle for OnWorldStateDirty / OnEitherDirty policies.
	//
	// In the unified ActionSet/Action model, Actions subscribe themselves at
	// activation time (ActionSet ChainUpdate processor) and unsubscribe at
	// deactivation. The root Action subscribes at AddAction time.
	// External consumers may also subscribe (e.g. for non-Action reactive
	// systems).
	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Add Subscriber")
	static FCk_Handle_Goap_WorldState
	Request_AddSubscriber(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		UPARAM(ref) FCk_Handle& InSubscriber);

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Remove Subscriber")
	static FCk_Handle_Goap_WorldState
	Request_RemoveSubscriber(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		UPARAM(ref) FCk_Handle& InSubscriber);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

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

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

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

	// ================================================================================================================
	// CAST
	// ================================================================================================================

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
	DoAddRequest(FCk_Handle_Goap_WorldState& InWorldState, const auto& InRequest) -> FCk_Handle_Goap_WorldState;

	// Tag every valid subscriber with FTag_Goap_Dirty_WorldState. Lazy-prune
	// invalid entries on the way. Lives on this Utils class (rather than as
	// a free function) so it has friend access to the Subscribers fragment's
	// private _Subscribers field. Used by the override-stack mutators when an
	// effective view change is detected.
	static auto
	DoTagSubscribersDirty(FCk_Handle_Goap_WorldState& InWorldState) -> void;
};

// ====================================================================================================================
