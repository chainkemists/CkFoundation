#pragma once

#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_ActionSet_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_ActionSet"))
class CKGOAP_API UCk_Utils_Goap_ActionSet_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_ActionSet_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_ActionSet);

public:
	// ================================================================================================================
	// CONSTRUCTION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Add ActionSet")
	static FCk_Handle_Goap_ActionSet
	AddActionSet(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_ActionSetParamsData& InParams);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Has")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Find Action")
	static FCk_Handle_Goap_Action
	Find_Action(
		const FCk_Handle_Goap_ActionSet& InActionSet,
		FGameplayTag InActionTag);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Get Active Chain")
	static TArray<FCk_Handle_Goap_Action>
	Get_ActiveChain(const FCk_Handle_Goap_ActionSet& InActionSet);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Get Enable Toggle")
	static ECk_EnableDisable
	Get_EnableToggle(const FCk_Handle_Goap_ActionSet& InActionSet);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Get Dependency Cycles")
	static TArray<FCk_GoapDiagnostic_DependencyCycle>
	Get_DependencyCycles(const FCk_Handle_Goap_ActionSet& InActionSet);

	// ================================================================================================================
	// REQUESTS
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Request Set Enable Toggle")
	static FCk_Handle_Goap_ActionSet
	Request_SetEnableToggle(
		UPARAM(ref) FCk_Handle_Goap_ActionSet& InActionSet,
		ECk_EnableDisable InToggle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Request Reset Active Chain")
	static FCk_Handle_Goap_ActionSet
	Request_ResetActiveChain(
		UPARAM(ref) FCk_Handle_Goap_ActionSet& InActionSet);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Bind To OnActiveChainChanged")
	static FCk_Handle_Goap_ActionSet
	BindTo_OnActiveChainChanged(
		UPARAM(ref) FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|ActionSet",
		DisplayName = "[Ck][Goap|ActionSet] Unbind From OnActiveChainChanged")
	static FCk_Handle_Goap_ActionSet
	UnbindFrom_OnActiveChainChanged(
		UPARAM(ref) FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate);
};
