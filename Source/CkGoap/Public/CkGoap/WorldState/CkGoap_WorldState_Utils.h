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
	// CREATION
	// ================================================================================================================
	//
	// Create — spawns a new child entity under InOwner that hosts a shared
	//          GOAP WorldState. The caller hands this handle to one or more
	//          GOAP planners via FCk_Fragment_Goap_ParamsData::_WorldStateSource.
	//          Reads, writes, and replan-trigger subscriptions on the planners
	//          all route through the resulting WorldState entity.
	//
	// Lifetime is cascade-bound to InOwner — destroying the owner destroys the
	// WorldState via the owner's RecordOfGoapWorldStates.

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
	// SUBSCRIBERS
	// ================================================================================================================
	//
	// Registering a planner as a subscriber on a WorldState causes that planner
	// to be tagged with FTag_Goap_Dirty_WorldState whenever a Set request on
	// this WorldState actually changes a key's value — which feeds the planner's
	// AutoReplan throttle for OnWorldStateDirty / OnEitherDirty policies.
	//
	// utils_goap::Add / Create call this automatically with the planner that
	// was just created. Manual callers don't typically need to invoke it —
	// it's exposed mainly for completeness and for callers that point an
	// already-existing planner at a different WorldState mid-life.

	UFUNCTION(BlueprintCallable, Category = "Ck|GOAP|WorldState",
		DisplayName = "[Ck][GOAP|WS] Request Add Planner Subscriber")
	static FCk_Handle_Goap_WorldState
	Request_AddPlannerSubscriber(
		UPARAM(ref) FCk_Handle_Goap_WorldState& InWorldState,
		UPARAM(ref) FCk_Handle_Goap& InPlanner);

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
};

// ====================================================================================================================
