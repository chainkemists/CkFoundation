#pragma once

#include "CkAStar_Test_Fragment_Data.h"
#include "CkAStar/CkAStar_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkAStar_Test_Utils.generated.h"

// ====================================================================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_AStarTest"))
class CKASTAR_API UCk_Utils_AStarTest_UE : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Utils_AStarTest_UE);
	CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AStarTest);

public:
	// ================================================================================================================
	// CREATION
	// ================================================================================================================

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Add Test Search")
	static FCk_Handle_AStarTest
	Add(
		UPARAM(ref) FCk_Handle& InOwner,
		int32 InGridWidth,
		int32 InGridHeight,
		int32 InStartX,
		int32 InStartY,
		int32 InGoalX,
		int32 InGoalY,
		int64 InBudgetMicroseconds = 0);

	// ================================================================================================================
	// CONTROL
	// ================================================================================================================

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Request Start Search")
	static FCk_Handle_AStarTest
	Request_StartSearch(
		UPARAM(ref) FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Request Block Cell")
	static void
	Request_BlockCell(
		UPARAM(ref) FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Request Unblock Cell")
	static void
	Request_UnblockCell(
		UPARAM(ref) FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY);

	// ================================================================================================================
	// QUERY
	// ================================================================================================================

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Search Status")
	static ECk_AStarSearchStatus
	Get_SearchStatus(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Path")
	static TArray<int32>
	Get_Path(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Total Cost")
	static float
	Get_TotalCost(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Total Iterations")
	static int32
	Get_TotalIterations(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Open Set Size")
	static int32
	Get_OpenSetSize(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Closed Set Size")
	static int32
	Get_ClosedSetSize(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Grid Width")
	static int32
	Get_GridWidth(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Get Grid Height")
	static int32
	Get_GridHeight(
		const FCk_Handle_AStarTest& InHandle);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Is Cell Blocked")
	static bool
	Is_CellBlocked(
		const FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY);

	UFUNCTION(BlueprintPure,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Has")
	static bool
	Has(
		const FCk_Handle& InHandle);

	// ================================================================================================================
	// CONFIG
	// ================================================================================================================

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Set Budget Microseconds")
	static void
	Set_BudgetMicroseconds(
		UPARAM(ref) FCk_Handle_AStarTest& InHandle,
		int64 InBudgetMicroseconds);

	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Set Cost Threshold")
	static void
	Set_CostThreshold(
		UPARAM(ref) FCk_Handle_AStarTest& InHandle,
		float InCostThreshold);

	// ================================================================================================================
	// CAST
	// ================================================================================================================

private:
	UFUNCTION(BlueprintCallable,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Cast",
		meta = (ExpandEnumAsExecs = "OutResult"))
	static FCk_Handle_AStarTest
	DoCast(
		UPARAM(ref) FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult);

	UFUNCTION(BlueprintPure,
		Category = "Ck|AStar|Test",
		DisplayName = "[Ck][AStar] Handle -> AStarTest Handle",
		meta = (CompactNodeTitle = "<AsAStarTest>", BlueprintAutocast))
	static FCk_Handle_AStarTest
	DoCastChecked(
		FCk_Handle InHandle);

	UFUNCTION(BlueprintPure,
		DisplayName = "[Ck] Get Invalid AStarTest Handle",
		Category = "Ck|AStar|Test",
		meta = (CompactNodeTitle = "INVALID_AStarTestHandle", Keywords = "make"))
	static FCk_Handle_AStarTest
	Get_InvalidHandle() { return {}; }
};

// ====================================================================================================================
