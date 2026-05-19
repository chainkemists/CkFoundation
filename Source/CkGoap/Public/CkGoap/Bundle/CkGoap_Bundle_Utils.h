#pragma once

#include "CkGoap/Bundle/CkGoap_Bundle_Fragment_Data.h"
#include "CkGoap/Tier/CkGoap_Tier_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGoap_Bundle_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Goap_Bundle"))
class CKGOAP_API UCk_Utils_Goap_Bundle_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_Goap_Bundle_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Goap_Bundle);

public:
	// ================================================================================================================
	// CONSTRUCTION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Add Bundle")
	static FCk_Handle_Goap_Bundle
	AddBundle(
		UPARAM(ref) FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_BundleParamsData& InParams);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Has")
	static bool
	Has(const FCk_Handle& InHandle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Find Tier")
	static FCk_Handle_Goap_Tier
	Find_Tier(
		const FCk_Handle_Goap_Bundle& InBundle,
		FGameplayTag InTierTag);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Get Active Tiers")
	static TArray<FCk_Handle_Goap_Tier>
	Get_ActiveTiers(const FCk_Handle_Goap_Bundle& InBundle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Get Enable Toggle")
	static ECk_EnableDisable
	Get_EnableToggle(const FCk_Handle_Goap_Bundle& InBundle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Get Dependency Cycles")
	static TArray<FCk_GoapDiagnostic_DependencyCycle>
	Get_DependencyCycles(const FCk_Handle_Goap_Bundle& InBundle);

	// ================================================================================================================
	// REQUESTS
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Request Set Enable Toggle")
	static FCk_Handle_Goap_Bundle
	Request_SetEnableToggle(
		UPARAM(ref) FCk_Handle_Goap_Bundle& InBundle,
		ECk_EnableDisable InToggle);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Request Reset Active Tiers")
	static FCk_Handle_Goap_Bundle
	Request_ResetActiveTiers(
		UPARAM(ref) FCk_Handle_Goap_Bundle& InBundle);

	// ================================================================================================================
	// SIGNAL BINDING
	// ================================================================================================================

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Bind To OnActiveTiersChanged")
	static FCk_Handle_Goap_Bundle
	BindTo_OnActiveTiersChanged(
		UPARAM(ref) FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
		ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

	UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Goap|Bundle",
		DisplayName = "[Ck][Goap|Bundle] Unbind From OnActiveTiersChanged")
	static FCk_Handle_Goap_Bundle
	UnbindFrom_OnActiveTiersChanged(
		UPARAM(ref) FCk_Handle_Goap_Bundle& InBundle,
		const FCk_Delegate_Goap_OnActiveTiersChanged& InDelegate);
};
