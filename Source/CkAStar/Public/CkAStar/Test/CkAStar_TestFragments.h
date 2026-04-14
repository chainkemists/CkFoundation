#pragma once

#include "CkAStar_Test_Fragment_Data.h"

#include "CkAStar/CkAStar_Fragment.h"
#include "CkAStar/CkAStar_Processor.h"
#include "CkAStar/Test/CkAStar_TestGraph.h"

#include "CkEcs/Scheduler/CkProcessorGroups.h"

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// CONCRETE FRAGMENT TYPES — Grid graph test instantiation
// ====================================================================================================================

using FFragment_AStarTest_SearchState = TFragment_AStar_SearchState<int32, astar::FGridGraph>;
using FFragment_AStarTest_Result = TFragment_AStar_Result<int32>;

// ====================================================================================================================
// GRID GRAPH FRAGMENT — Holds the grid graph instance (shared across search resets)
// ====================================================================================================================

struct CKASTAR_API FFragment_AStarTest_GridGraph
{
public:
	CK_GENERATED_BODY(FFragment_AStarTest_GridGraph);

	friend class UCk_Utils_AStarTest_UE;

	astar::FGridGraph _Graph;
	int32 _StartNode = 0;
};

// ====================================================================================================================
// CONCRETE PROCESSORS — Inherit from base templates, add scheduling
// ====================================================================================================================

struct FProcessor_AStarTest_Execute
	: TProcessor_AStar_Execute<FProcessor_AStarTest_Execute,
		  FCk_Handle_AStarTest, FFragment_AStarTest_SearchState, FFragment_AStarTest_Result>
{
	using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
	using Group = FGroup_Gameplay_AI;
};

// --------------------------------------------------------------------------------------------------------------------

struct FProcessor_AStarTest_EndPlay
	: TProcessor_AStar_EndPlay<FProcessor_AStarTest_EndPlay,
		  FCk_Handle_AStarTest, FFragment_AStarTest_SearchState>
{
	using TProcessor_AStar_EndPlay::TProcessor_AStar_EndPlay;
	using Group = FGroup_PreDestruction;
};

// ====================================================================================================================

} // namespace ck
