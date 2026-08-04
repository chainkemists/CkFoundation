#pragma once

#include "CkAStar/Algorithm/CkAStar_GraphConcept.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Types.h"
#include "CkVoxelNav/Path/CkVoxelNavPath_Fragment_Data.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// The A* search space over one baked octree: an FCellId-keyed graph view plus the one synchronous,
// iteration-capped search that runs on it.
//
// LIFETIME: FPathGraph holds a raw pointer to a published octree and is valid only for one synchronous
// search inside a processor tick. Never store it across frames. Its caller must hold the octree's
// TSharedPtr for the whole search - the volume swaps that pointer atomically when a rebuild completes,
// and a copy of it is what keeps the structure the search is walking whole.
//
// The graph never unpacks an FCellId. Everything it asks about a cell goes through the octree's
// kind-dispatching cell seam (Get_Cell*), which is what lets a coarser cell representation drop in
// later without touching search, cost, or neighbour enumeration.
//
// Derived from Nav3D 2.0, (c) 2025 Darby Costello, MIT.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    /** Per-query, cell-independent seed. Immutable once built, so concurrent searches over the same
     *  octree may share one instance. */
    struct CKVOXELNAV_API FPathGraphSharedData
    {
        FVector _GoalLocation = FVector::ZeroVector;
        FCellId _GoalCell;

        // An agent of radius R fits only a cell whose half-extent is at least R. Expressed as an extent
        // rather than a layer index because search must stay blind to how a cell is addressed.
        float _MinCellExtentUu = 0.0f;

        float _HeuristicScale = 1.0f;

        ECk_EnableDisable _NodeSizeCompensation = ECk_EnableDisable::Disable;

        // The compensation baseline: the finest cell the bake carries. Zero disables compensation.
        float _FinestCellExtentUu = 0.0f;
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FPathGraph
    {
    public:
        FPathGraph() = default;

        FPathGraph(
            const FOctree* InOctree,
            FVolumeId InVolume,
            TSharedPtr<const FPathGraphSharedData> InShared);

    public:
        auto
        Neighbors(
            const FCellId& InCell) const -> TArray<FCellId>;

        auto
        Cost(
            const FCellId& InFrom,
            const FCellId& InTo) const -> float;

        auto
        Heuristic(
            const FCellId& InCurrent,
            const FCellId& InGoal) const -> float;

        auto
        IsGoal(
            const FCellId& InCell) const -> bool;

    public:
        auto
        Get_CellLocation(
            const FCellId& InCell) const -> FVector;

        auto
        Get_IsValid() const -> bool;

    private:
        auto
        DoGet_NodeSizeCompensation(
            const FCellId& InCell) const -> float;

    private:
        // LIFETIME: see the header note - one synchronous search inside one processor tick, never stored.
        const FOctree* _Octree = nullptr;
        FVolumeId _Volume;
        TSharedPtr<const FPathGraphSharedData> _Shared;
    };

    // ----------------------------------------------------------------------------------------------------------------

    static_assert(
        astar::AStarGraph<FPathGraph, FCellId>,
        "FPathGraph must satisfy the AStarGraph concept");

    // ----------------------------------------------------------------------------------------------------------------

    struct CKVOXELNAV_API FPathSearchParams
    {
        FVector _From = FVector::ZeroVector;
        FVector _To = FVector::ZeroVector;

        float _AgentRadiusUu = 0.0f;
        float _HeuristicScale = 1.0f;

        ECk_EnableDisable _NodeSizeCompensation = ECk_EnableDisable::Disable;

        // Hard cap on expansions. The search is synchronous, so this is the only thing standing between a
        // pathological query and a stalled frame.
        int32 _MaxIterations = 0;

        // The volume's build epoch at plan time, carried into the result so staleness stays detectable.
        int32 _PlannedAgainstEpoch = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** A finished plan. `_Cells` is the search's own answer and `_Waypoints` is what a follower consumes:
     *  the From position, every cell centre in order, then the To position. Both are kept because the
     *  cells are what a later validation pass (`astar::ValidateExistingPath`) can re-check, and the
     *  positions are what movement code actually flies. */
    struct CKVOXELNAV_API FPathPlan
    {
        ECk_VoxelNav_PathSearchOutcome _Outcome = ECk_VoxelNav_PathSearchOutcome::InvalidRequest;

        TArray<FCellId> _Cells;
        TArray<FVector> _Waypoints;

        float _TotalCost = 0.0f;
        int32 _Iterations = 0;
        int32 _PlannedAgainstEpoch = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** One synchronous, iteration-capped A* over the published octree. Endpoints are resolved to free
     *  cells first (a position buried in geometry snaps to the nearest free cell of its leaf), so a
     *  caller never has to pre-validate them. */
    CKVOXELNAV_API auto
    Search_PathGraph(
        const FOctree& InOctree,
        FVolumeId InVolume,
        const FPathSearchParams& InParams) -> FPathPlan;

    /** True when the volume has rebuilt since this plan was made. Any difference counts, in either
     *  direction: a volume that swapped its bake makes no promise that the old epoch's free space
     *  survived, and epochs are not comparable across two different volumes. */
    CKVOXELNAV_API auto
    Get_IsPlannedEpochStale(
        int32 InPlannedAgainstEpoch,
        int32 InCurrentEpoch) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
