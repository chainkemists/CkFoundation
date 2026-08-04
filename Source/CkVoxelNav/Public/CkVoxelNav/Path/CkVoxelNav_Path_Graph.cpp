#include "CkVoxelNav_Path_Graph.h"

#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_path_graph
{
    // Six faces, and a face abutting a subdivided neighbour contributes up to sixteen leaf sub-nodes.
    constexpr auto TypicalNeighborCount = 24;

    auto
        TryGet_FreeCellAtPosition(
            const ck::voxelnav::FOctree& InOctree,
            const FVector& InPosition,
            ck::voxelnav::LayerIndex InMinLayerIndex,
            ck::voxelnav::FVolumeId InVolume)
        -> ck::voxelnav::FCellId
    {
        // Snaps a position buried in geometry to the nearest free cell of its leaf, and falls back to a
        // whole-octree nearest-free scan - which is exactly what resolving a path endpoint wants.
        const auto Lookup = TryGet_NodeAddressFromPosition(InOctree, InPosition, InMinLayerIndex);

        if (Lookup._Outcome != ECk_VoxelNav_AddressLookup::Found)
        { return {}; }

        return ck::voxelnav::FCellId::Make_FromNodeAddress(InVolume, Lookup._Address);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    FPathGraph::
        FPathGraph(
            const FOctree* InOctree,
            FVolumeId InVolume,
            TSharedPtr<const FPathGraphSharedData> InShared)
        : _Octree(InOctree)
        , _Volume(InVolume)
        , _Shared(MoveTemp(InShared))
    {
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPathGraph::
        Neighbors(
            const FCellId& InCell) const
        -> TArray<FCellId>
    {
        using namespace ck_voxelnav_path_graph;

        auto Neighbors = TArray<FCellId>{};

        if (NOT Get_IsValid())
        { return Neighbors; }

        Neighbors.Reserve(TypicalNeighborCount);

        // The octree's neighbour walk already drops occluded cells and handles the parent/child
        // transitions across a layer change; the agent-size filter is the only thing left to apply.
        Get_CellNeighbors(*_Octree, InCell, Neighbors);

        const auto MinCellExtentUu = _Shared->_MinCellExtentUu;

        if (MinCellExtentUu <= 0.0f)
        { return Neighbors; }

        Neighbors.RemoveAll([&](const FCellId& InNeighbor) -> bool
        {
            return Get_CellExtent(*_Octree, InNeighbor) < MinCellExtentUu;
        });

        return Neighbors;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPathGraph::
        Cost(
            const FCellId& InFrom,
            const FCellId& InTo) const
        -> float
    {
        if (NOT Get_IsValid())
        { return 0.0f; }

        const auto Distance = static_cast<float>(
            FVector::Dist(Get_CellLocation(InFrom), Get_CellLocation(InTo)));

        return Distance * DoGet_NodeSizeCompensation(InTo);
    }

    auto
        FPathGraph::
        Heuristic(
            const FCellId& InCurrent,
            const FCellId& InGoal) const
        -> float
    {
        if (NOT Get_IsValid())
        { return 0.0f; }

        const auto Distance = static_cast<float>(
            FVector::Dist(Get_CellLocation(InCurrent), _Shared->_GoalLocation));

        return Distance * _Shared->_HeuristicScale;
    }

    auto
        FPathGraph::
        IsGoal(
            const FCellId& InCell) const
        -> bool
    {
        if (NOT Get_IsValid())
        { return false; }

        // Cell equality, not position proximity: the search is over cells, and the caller already
        // resolved the goal position to one before building this graph.
        return InCell == _Shared->_GoalCell;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPathGraph::
        Get_CellLocation(
            const FCellId& InCell) const
        -> FVector
    {
        if (NOT Get_IsValid())
        { return FVector::ZeroVector; }

        return Get_CellCenter(*_Octree, InCell);
    }

    auto
        FPathGraph::
        Get_IsValid() const
        -> bool
    {
        return ck::IsValid(_Octree, ck::IsValid_Policy_NullptrOnly{}) && _Shared.IsValid();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FPathGraph::
        DoGet_NodeSizeCompensation(
            const FCellId& InCell) const
        -> float
    {
        constexpr auto NoCompensation = 1.0f;

        if (_Shared->_NodeSizeCompensation == ECk_EnableDisable::Disable)
        { return NoCompensation; }

        const auto FinestCellExtentUu = _Shared->_FinestCellExtentUu;

        if (FinestCellExtentUu <= 0.0f)
        { return NoCompensation; }

        const auto ExtentRatio = Get_CellExtent(*_Octree, InCell) / FinestCellExtentUu;

        return NoCompensation / FMath::Max(NoCompensation, ExtentRatio);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Search_PathGraph(
            const FOctree& InOctree,
            FVolumeId InVolume,
            const FPathSearchParams& InParams)
        -> FPathPlan
    {
        using namespace ck_voxelnav_path_graph;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Search_PathGraph);

        auto Plan = FPathPlan{};
        Plan._PlannedAgainstEpoch = InParams._PlannedAgainstEpoch;

        const auto RequestIsValid =
            NOT InParams._From.ContainsNaN() &&
            NOT InParams._To.ContainsNaN() &&
            InParams._MaxIterations > 0 &&
            InParams._HeuristicScale > 0.0f;

        if (NOT RequestIsValid)
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::InvalidRequest;
            return Plan;
        }

        if (NOT InOctree.Get_IsValid())
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::NoBake;
            return Plan;
        }

        const auto MinLayerIndex = Get_MinLayerIndexForAgentRadius(InOctree, InParams._AgentRadiusUu);

        const auto StartCell = TryGet_FreeCellAtPosition(InOctree, InParams._From, MinLayerIndex, InVolume);
        const auto GoalCell = TryGet_FreeCellAtPosition(InOctree, InParams._To, MinLayerIndex, InVolume);

        if (NOT StartCell.Get_IsValid() || NOT GoalCell.Get_IsValid())
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::EndpointUnresolvable;
            return Plan;
        }

        auto SharedData = FPathGraphSharedData{};

        SharedData._GoalLocation = Get_CellCenter(InOctree, GoalCell);
        SharedData._GoalCell = GoalCell;
        SharedData._MinCellExtentUu = InParams._AgentRadiusUu;
        SharedData._HeuristicScale = InParams._HeuristicScale;
        SharedData._NodeSizeCompensation = InParams._NodeSizeCompensation;
        SharedData._FinestCellExtentUu = InOctree.Get_LeafNodes().Get_LeafSubNodeExtent();

        const auto Shared = TSharedPtr<const FPathGraphSharedData>{
            MakeShared<FPathGraphSharedData>(MoveTemp(SharedData))};

        const auto Graph = FPathGraph{&InOctree, InVolume, Shared};

        auto Search = astar::TSearchState<FCellId, FPathGraph>{Graph, StartCell, GoalCell};

        auto SearchParams = astar::FSearchParams{};
        SearchParams.MaxIterationsPerTick = InParams._MaxIterations;

        const auto SearchStatus = Search.ContinueSearch(SearchParams);

        Plan._Iterations = Search.GetTotalIterations();

        if (SearchStatus == astar::ESearchStatus::InProgress)
        {
            // The cap is the only thing bounding a synchronous search, so hitting it is a distinct
            // answer from "no path exists" - the caller may retry with a bigger budget.
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::IterationCapReached;
            return Plan;
        }

        if (SearchStatus != astar::ESearchStatus::Complete)
        {
            Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::Unreachable;
            return Plan;
        }

        Plan._Outcome = ECk_VoxelNav_PathSearchOutcome::Succeeded;
        Plan._Cells = Search.GetResultPath();
        Plan._TotalCost = Search.GetResultCost();

        constexpr auto EndpointCount = 2;
        Plan._Waypoints.Reserve(Plan._Cells.Num() + EndpointCount);

        Plan._Waypoints.Emplace(InParams._From);

        for (const auto& Cell : Plan._Cells)
        { Plan._Waypoints.Emplace(Get_CellCenter(InOctree, Cell)); }

        Plan._Waypoints.Emplace(InParams._To);

        return Plan;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsPlannedEpochStale(
            int32 InPlannedAgainstEpoch,
            int32 InCurrentEpoch)
        -> bool
    {
        return InPlannedAgainstEpoch != InCurrentEpoch;
    }
}

// --------------------------------------------------------------------------------------------------------------------
