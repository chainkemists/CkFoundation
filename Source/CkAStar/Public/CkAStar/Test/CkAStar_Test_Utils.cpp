#include "CkAStar_Test_Utils.h"

#include "CkAStar/Test/CkAStar_TestFragments.h"
#include "CkAStar/CkAStar_Fragment.h"
#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Add(
		FCk_Handle& InOwner,
		int32 InGridWidth,
		int32 InGridHeight,
		int32 InStartX,
		int32 InStartY,
		int32 InGoalX,
		int32 InGoalY,
		int64 InBudgetMicroseconds)
	-> FCk_Handle_AStarTest
{
	const auto StartNode = InStartY * InGridWidth + InStartX;
	const auto GoalNode = InGoalY * InGridWidth + InGoalX;

	auto Graph = ck::astar::FGridGraph{InGridWidth, InGridHeight, GoalNode};

	auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewEntity)
	{
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
		UCk_Utils_Handle_UE::Set_DebugName(InNewEntity, "AStarTest");
#endif

		auto GridGraphFragment = ck::FFragment_AStarTest_GridGraph{};
		GridGraphFragment._Graph = Graph;
		GridGraphFragment._StartNode = StartNode;
		InNewEntity.Add<ck::FFragment_AStarTest_GridGraph>(GridGraphFragment);

		auto Params = ck::FFragment_AStar_Params{};
		Params.Set_BudgetMicroseconds(InBudgetMicroseconds);
		InNewEntity.Add<ck::FFragment_AStar_Params>(Params);

		InNewEntity.Add<ck::FFragment_AStarTest_SearchState>();
		InNewEntity.Add<ck::FFragment_AStarTest_Result>();
		InNewEntity.Add<ck::FFragment_AStar_Debug>();
	});

	return ck::StaticCast<FCk_Handle_AStarTest>(NewEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Request_StartSearch(
		FCk_Handle_AStarTest& InHandle)
	-> FCk_Handle_AStarTest
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Request_StartSearch"))
	{ return InHandle; }

	auto& GridGraphFragment = InHandle.Get<ck::FFragment_AStarTest_GridGraph>();
	auto& SearchState = InHandle.Get<ck::FFragment_AStarTest_SearchState>();
	auto& Result = InHandle.Get<ck::FFragment_AStarTest_Result>();

	const auto& Graph = GridGraphFragment._Graph;
	const auto StartNode = GridGraphFragment._StartNode;
	const auto GoalNode = Graph.Get_GoalNode();
	const auto& Params = InHandle.Get<ck::FFragment_AStar_Params>();

	SearchState._State = ck::astar::TSearchState<int32, ck::astar::FGridGraph>{
		Graph, StartNode, GoalNode};

	Result._SearchStatus = ECk_AStarSearchStatus::InProgress;
	Result._Path.Reset();
	Result._TotalCost = 0.0f;
	Result._TotalIterations = 0;
	Result._TotalTimeMicroseconds = 0;

	InHandle.Try_Remove<ck::FTag_AStar_SearchComplete>();
	InHandle.Add<ck::FTag_AStar_SearchActive>();

	return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Request_BlockCell(
		FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY)
	-> void
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Request_BlockCell"))
	{ return; }

	auto& GridGraphFragment = InHandle.Get<ck::FFragment_AStarTest_GridGraph>();
	const auto CellIndex = GridGraphFragment._Graph.XYToCell(InX, InY);
	GridGraphFragment._Graph.BlockCell(CellIndex);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Request_UnblockCell(
		FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY)
	-> void
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Request_UnblockCell"))
	{ return; }

	auto& GridGraphFragment = InHandle.Get<ck::FFragment_AStarTest_GridGraph>();
	const auto CellIndex = GridGraphFragment._Graph.XYToCell(InX, InY);
	GridGraphFragment._Graph.UnblockCell(CellIndex);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_SearchStatus(
		const FCk_Handle_AStarTest& InHandle)
	-> ECk_AStarSearchStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_SearchStatus"))
	{ return ECk_AStarSearchStatus::Idle; }

	return InHandle.Get<ck::FFragment_AStarTest_Result>()._SearchStatus;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_Path(
		const FCk_Handle_AStarTest& InHandle)
	-> TArray<int32>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_Path"))
	{ return {}; }

	return InHandle.Get<ck::FFragment_AStarTest_Result>()._Path;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_TotalCost(
		const FCk_Handle_AStarTest& InHandle)
	-> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_TotalCost"))
	{ return 0.0f; }

	return InHandle.Get<ck::FFragment_AStarTest_Result>()._TotalCost;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_TotalIterations(
		const FCk_Handle_AStarTest& InHandle)
	-> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_TotalIterations"))
	{ return 0; }

	return InHandle.Get<ck::FFragment_AStarTest_Result>()._TotalIterations;
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_OpenSetSize(
		const FCk_Handle_AStarTest& InHandle)
	-> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_OpenSetSize"))
	{ return 0; }

	if (NOT InHandle.Has<ck::FFragment_AStar_Debug>())
	{ return 0; }

	return InHandle.Get<ck::FFragment_AStar_Debug>().Get_OpenSetSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_ClosedSetSize(
		const FCk_Handle_AStarTest& InHandle)
	-> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_ClosedSetSize"))
	{ return 0; }

	if (NOT InHandle.Has<ck::FFragment_AStar_Debug>())
	{ return 0; }

	return InHandle.Get<ck::FFragment_AStar_Debug>().Get_ClosedSetSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_GridWidth(
		const FCk_Handle_AStarTest& InHandle)
	-> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_GridWidth"))
	{ return 0; }

	return InHandle.Get<ck::FFragment_AStarTest_GridGraph>()._Graph.Get_Width();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Get_GridHeight(
		const FCk_Handle_AStarTest& InHandle)
	-> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Get_GridHeight"))
	{ return 0; }

	return InHandle.Get<ck::FFragment_AStarTest_GridGraph>()._Graph.Get_Height();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Is_CellBlocked(
		const FCk_Handle_AStarTest& InHandle,
		int32 InX,
		int32 InY)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Is_CellBlocked"))
	{ return false; }

	const auto& Graph = InHandle.Get<ck::FFragment_AStarTest_GridGraph>()._Graph;
	return Graph.IsCellBlocked(Graph.XYToCell(InX, InY));
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Has(
		const FCk_Handle& InHandle)
	-> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_AStarTest_GridGraph>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Set_BudgetMicroseconds(
		FCk_Handle_AStarTest& InHandle,
		int64 InBudgetMicroseconds)
	-> void
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Set_BudgetMicroseconds"))
	{ return; }

	InHandle.Get<ck::FFragment_AStar_Params>().Set_BudgetMicroseconds(InBudgetMicroseconds);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	Set_CostThreshold(
		FCk_Handle_AStarTest& InHandle,
		float InCostThreshold)
	-> void
{
	CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid handle in Set_CostThreshold"))
	{ return; }

	InHandle.Get<ck::FFragment_AStar_Params>().Set_CostThreshold(InCostThreshold);
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	DoCast(
		FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult)
	-> FCk_Handle_AStarTest
{
	if (Has(InHandle))
	{
		OutResult = ECk_SucceededFailed::Succeeded;
		return ck::StaticCast<FCk_Handle_AStarTest>(InHandle);
	}

	OutResult = ECk_SucceededFailed::Failed;
	return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_AStarTest_UE::
	DoCastChecked(
		FCk_Handle InHandle)
	-> FCk_Handle_AStarTest
{
	return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
