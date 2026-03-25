#pragma once

#include "CkStateTree/CkStateTree_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkStateTree_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UStateTree;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_StateTree"))
class CKSTATETREE_API UCk_Utils_StateTree_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_StateTree_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_StateTree);

public:
	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Add Feature")
	static FCk_Handle_StateTree
	Add(
		UPARAM(ref) FCk_Handle& InHandle,
		const FCk_Fragment_StateTree_ParamsData& InParams);

	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Has Feature")
	static bool
	Has(
		const FCk_Handle& InHandle);

private:
	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Cast",
			  meta = (ExpandEnumAsExecs = "OutResult"))
	static FCk_Handle_StateTree
	DoCast(
		UPARAM(ref) FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult);

	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Handle -> StateTree Handle",
			  meta = (CompactNodeTitle = "<AsStateTree>", BlueprintAutocast))
	static FCk_Handle_StateTree
	DoCastChecked(
		FCk_Handle InHandle);

	UFUNCTION(BlueprintPure,
			  DisplayName = "[Ck] Get Invalid StateTree Handle",
			  Category = "Ck|Utils|StateTree",
			  meta = (CompactNodeTitle = "INVALID_StateTreeHandle", Keywords = "make"))
	static FCk_Handle_StateTree
	Get_InvalidHandle() { return {}; };

	// ----------------------------------------------------------------------------------------------------------------
	// Lifecycle Request Functions
	// ----------------------------------------------------------------------------------------------------------------

public:
	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Request Start Logic")
	static FCk_Handle_StateTree
	Request_StartLogic(
		UPARAM(ref) FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_StartLogic& InRequest);

	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Request Restart Logic")
	static FCk_Handle_StateTree
	Request_RestartLogic(
		UPARAM(ref) FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_RestartLogic& InRequest);

	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Request Stop Logic")
	static FCk_Handle_StateTree
	Request_StopLogic(
		UPARAM(ref) FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_StopLogic& InRequest);

	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Request Pause Logic")
	static FCk_Handle_StateTree
	Request_PauseLogic(
		UPARAM(ref) FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_PauseLogic& InRequest);

	UFUNCTION(BlueprintCallable,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Request Resume Logic")
	static FCk_Handle_StateTree
	Request_ResumeLogic(
		UPARAM(ref) FCk_Handle_StateTree& InHandle,
		const FCk_Request_StateTree_ResumeLogic& InRequest);

	// ----------------------------------------------------------------------------------------------------------------
	// State Query Functions
	// ----------------------------------------------------------------------------------------------------------------

public:
	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Is Running")
	static bool
	Get_IsRunning(
		const FCk_Handle_StateTree& InHandle);

	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Is Paused")
	static bool
	Get_IsPaused(
		const FCk_Handle_StateTree& InHandle);

	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Get Run Status")
	static ECk_StateTree_RunStatus
	Get_RunStatus(
		const FCk_Handle_StateTree& InHandle);

	UFUNCTION(BlueprintPure,
			  Category = "Ck|Utils|StateTree",
			  DisplayName = "[Ck][StateTree] Get StateTree Asset")
	static UStateTree*
	Get_StateTreeAsset(
		const FCk_Handle_StateTree& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
