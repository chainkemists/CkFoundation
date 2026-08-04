#include "CkVoxelNav_Octree_Build.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Merge.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Morton.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

#include <HAL/PlatformTime.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_build
{
    // Everything from NotStarted up to (not including) Complete, which is what a progress fraction
    // normalizes against.
    constexpr auto WorkingStageCount = static_cast<int32>(ECk_VoxelNav_BuildStage::Complete);

    // The first layer built from blocked parents rather than by the leaf stages.
    constexpr auto FirstInteriorLayerIndex = 1;

    auto
        Get_StageSpendsProbes(
            ECk_VoxelNav_BuildStage InStage)
        -> bool
    {
        return InStage == ECk_VoxelNav_BuildStage::RasterizeL1 ||
               InStage == ECk_VoxelNav_BuildStage::ProbeLeafOccupancy ||
               InStage == ECk_VoxelNav_BuildStage::RasterizeSubNodes;
    }

    /** Everything a step can move without leaving its stage. The two layer-walking stages advance only
     *  `_LayerCursor` and spend no probe, so a stage-and-probes comparison alone would read a working
     *  step as a stalled one. */
    struct FStepFootprint
    {
        ECk_VoxelNav_BuildStage _Stage = ECk_VoxelNav_BuildStage::NotStarted;
        int32 _Cursor = 0;
        int32 _SubCursor = 0;
        int32 _LayerCursor = 0;

        auto operator==(const FStepFootprint&) const -> bool = default;
    };

    auto
        Get_StepFootprint(
            ECk_VoxelNav_BuildStage InStage,
            const ck::voxelnav::FRasterizeScratch& InScratch)
        -> FStepFootprint
    {
        return FStepFootprint{InStage, InScratch._Cursor, InScratch._SubCursor, InScratch._LayerCursor};
    }

    auto
        Get_TotalNodeCount(
            const ck::voxelnav::FOctree& InOctree)
        -> int32
    {
        auto NodeCount = 0;

        for (const auto& Layer : InOctree.Get_Layers())
        { NodeCount += Layer.Get_NodeCount(); }

        return NodeCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        FBuildState::
        Get_Progress01() const
        -> float
    {
        using namespace ck_voxelnav_octree_build;

        if (_Stage == ECk_VoxelNav_BuildStage::Complete)
        { return 1.0f; }

        // A failed build published nothing, so there is no progress a consumer can act on.
        if (_Stage == ECk_VoxelNav_BuildStage::Failed)
        { return 0.0f; }

        return static_cast<float>(static_cast<int32>(_Stage)) / static_cast<float>(WorkingStageCount);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_ResetBuild(
            FBuildState& InOutState)
        -> void
    {
        InOutState._Stage = ECk_VoxelNav_BuildStage::NotStarted;
        InOutState._Stats = FCk_VoxelNav_BuildStats{};
        InOutState._StartSeconds = 0.0;

        Request_ResetOctree(InOutState._Octree);
        Request_ResetScratch(InOutState._Scratch);
    }

    auto
        Request_FailBuild(
            FBuildState& InOutState)
        -> void
    {
        InOutState._Stage = ECk_VoxelNav_BuildStage::Failed;

        Request_ResetOctree(InOutState._Octree);
        Request_ResetScratch(InOutState._Scratch);
    }

    auto
        Request_ReleaseBuiltOctree(
            FBuildState& InOutState)
        -> TSharedPtr<const FOctree>
    {
        if (InOutState._Stage != ECk_VoxelNav_BuildStage::Complete)
        { return {}; }

        auto Published = MakeShared<FOctree>(MoveTemp(InOutState._Octree));

        Request_ResetOctree(InOutState._Octree);
        Request_ResetScratch(InOutState._Scratch);

        return Published;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_AdvanceBuild(
            FBuildState& InOutState,
            const FBuildParams& InParams,
            const ICk_VoxelNav_GeometryBackend& InBackend,
            const FBuildBudget& InBudget)
        -> void
    {
        using namespace ck_voxelnav_octree_build;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Request_AdvanceBuild);

        if (InOutState.Get_IsFinished())
        { return; }

        if (InOutState._Stage == ECk_VoxelNav_BuildStage::NotStarted)
        { InOutState._StartSeconds = FPlatformTime::Seconds(); }

        // Bound once here rather than reached through the state below, so the stage machine reads as plain
        // operations on a stage, an octree, a scratch and a counter set.
        auto& BuildStage = InOutState._Stage;
        auto& WorkingOctree = InOutState._Octree;
        auto& Scratch = InOutState._Scratch;
        auto& Stats = InOutState._Stats;

        // Merging closes the bake rather than taking a stage of its own: it spends no probe, cannot resume,
        // and a stage would put a slice boundary either side of a pass that has to run to completion anyway.
        // Its cost is RECORDED instead - _MergeSeconds is what says whether it ever needs budgeting.
        const auto DoMergeCells = [&]() -> void
        {
            if (InParams._CellMerging != ECk_EnableDisable::Enable)
            { return; }

            const auto MergeStartSeconds = FPlatformTime::Seconds();

            Request_MergeCells(WorkingOctree);

            const auto& MergedCells = WorkingOctree.Get_MergedCells();

            Stats.Set_PlainCellCount(MergedCells.Get_SourceCellCount());
            Stats.Set_MergedCellCount(MergedCells.Get_CellCount());
            Stats.Set_MergeSeconds(static_cast<float>(FPlatformTime::Seconds() - MergeStartSeconds));
        };

        const auto DoFinishBuild = [&]() -> void
        {
            DoMergeCells();

            Request_MarkOctreeBuilt(WorkingOctree);

            Stats.Set_TotalNodeCount(Get_TotalNodeCount(WorkingOctree));
            BuildStage = ECk_VoxelNav_BuildStage::Complete;
        };

        // Returns how many occupancy probes the step spent, and moves the stage on when the step finished
        // it. Every non-terminal branch either transitions or spends a probe, which is what lets the slice
        // loop below treat "neither happened" as a broken invariant rather than as normal.
        const auto DoAdvanceOneStep = [&](int32 InProbeBudget) -> int32
        {
            switch (BuildStage)
            {
                case ECk_VoxelNav_BuildStage::NotStarted:
                {
                    BuildStage = ECk_VoxelNav_BuildStage::InitializeOctree;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::InitializeOctree:
                {
                    const auto OctreeInitialized = Request_InitializeOctree(
                        WorkingOctree, InParams._FinestCellSizeUu, InParams._VolumeBounds);

                    if (NOT OctreeInitialized)
                    {
                        BuildStage = ECk_VoxelNav_BuildStage::Failed;
                        return 0;
                    }

                    Request_InitializeScratch(WorkingOctree, Scratch);

                    BuildStage = ECk_VoxelNav_BuildStage::ProbeVolumeEmpty;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::ProbeVolumeEmpty:
                {
                    const auto VolumeProbe = Stage_ProbeVolumeEmpty(WorkingOctree, InBackend);

                    Stats.Set_BroadphaseBodyCount(VolumeProbe._BodyCount);

                    // An empty volume is entirely free space: every layer stays nodeless, which is exactly
                    // what a query reads as navigable. Rasterizing it would spend a full budget arriving at
                    // the same answer.
                    if (VolumeProbe._VolumeIsEmpty)
                    {
                        DoFinishBuild();
                        return 0;
                    }

                    BuildStage = ECk_VoxelNav_BuildStage::RasterizeL1;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::RasterizeL1:
                {
                    const auto StageResult = Stage_RasterizeL1(
                        WorkingOctree, Scratch, InBackend, InParams._ClearanceUu, InProbeBudget);

                    if (StageResult._StageComplete)
                    {
                        Stats.Set_BlockedLayer1Cells(Scratch._BlockedNodes[0].Num());
                        Scratch._Cursor = 0;
                        BuildStage = ECk_VoxelNav_BuildStage::PropagateBlocked;
                    }

                    return StageResult._ProbesSpent;
                }
                case ECk_VoxelNav_BuildStage::PropagateBlocked:
                {
                    Stage_PropagateBlockedUpward(WorkingOctree, Scratch);

                    BuildStage = ECk_VoxelNav_BuildStage::AllocateLeaves;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::AllocateLeaves:
                {
                    Stage_AllocateLeaves(WorkingOctree, Scratch);

                    BuildStage = ECk_VoxelNav_BuildStage::ProbeLeafOccupancy;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::ProbeLeafOccupancy:
                {
                    const auto StageResult = Stage_ProbeLeafOccupancy(
                        WorkingOctree, Scratch, InBackend, InParams._ClearanceUu, InProbeBudget);

                    if (StageResult._StageComplete)
                    {
                        Scratch._Cursor = 0;
                        BuildStage = ECk_VoxelNav_BuildStage::SortLeafLayer;
                    }

                    return StageResult._ProbesSpent;
                }
                case ECk_VoxelNav_BuildStage::SortLeafLayer:
                {
                    Stage_SortLeafLayer(WorkingOctree, Scratch);

                    Stats.Set_LeafNodeCount(WorkingOctree.Get_Layer(0).Get_NodeCount());
                    Stats.Set_OccludedLeafCount(Scratch._OccludedLeafMortons.Num());

                    BuildStage = ECk_VoxelNav_BuildStage::RasterizeSubNodes;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::RasterizeSubNodes:
                {
                    const auto StageResult = Stage_RasterizeLeafSubNodes(
                        WorkingOctree, Scratch, InBackend, InParams._ClearanceUu, InProbeBudget);

                    if (StageResult._StageComplete)
                    {
                        Scratch._Cursor = 0;
                        Scratch._SubCursor = 0;
                        Scratch._LayerCursor = FirstInteriorLayerIndex;
                        BuildStage = ECk_VoxelNav_BuildStage::RasterizeLayers;
                    }

                    return StageResult._ProbesSpent;
                }
                case ECk_VoxelNav_BuildStage::RasterizeLayers:
                {
                    if (Scratch._LayerCursor >= WorkingOctree.Get_LayerCount())
                    {
                        BuildStage = ECk_VoxelNav_BuildStage::BuildParentLinks;
                        return 0;
                    }

                    Stage_RasterizeLayer(WorkingOctree, Scratch,
                        static_cast<LayerIndex>(Scratch._LayerCursor));

                    ++Scratch._LayerCursor;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::BuildParentLinks:
                {
                    Stage_BuildParentLinks(WorkingOctree, Scratch);

                    // The neighbour walk climbs to a parent when a step leaves the layer, so the topmost
                    // layer has nothing to climb to and is skipped.
                    Scratch._LayerCursor = WorkingOctree.Get_LayerCount() - 2;
                    BuildStage = ECk_VoxelNav_BuildStage::BuildNeighbourLinks;
                    return 0;
                }
                case ECk_VoxelNav_BuildStage::BuildNeighbourLinks:
                {
                    if (Scratch._LayerCursor < 0)
                    {
                        DoFinishBuild();
                        return 0;
                    }

                    Stage_BuildNeighbourLinks(WorkingOctree, Scratch,
                        static_cast<LayerIndex>(Scratch._LayerCursor));

                    --Scratch._LayerCursor;
                    return 0;
                }
                default:
                {
                    return 0;
                }
            }
        };

        auto ProbesRemaining = InBudget._MaxOccupancyProbes;
        const auto SliceStartSeconds = FPlatformTime::Seconds();

        for (;;)
        {
            if (Get_StageSpendsProbes(BuildStage) && ProbesRemaining <= 0)
            { break; }

            if (InBudget._MaxSeconds > 0.0f &&
                FPlatformTime::Seconds() - SliceStartSeconds >= static_cast<double>(InBudget._MaxSeconds))
            { break; }

            const auto FootprintBeforeStep = Get_StepFootprint(BuildStage, Scratch);
            const auto ProbesSpent = DoAdvanceOneStep(ProbesRemaining);

            ProbesRemaining -= ProbesSpent;
            Stats.Set_OccupancyProbes(Stats.Get_OccupancyProbes() + ProbesSpent);

            if (InOutState.Get_IsFinished())
            { break; }

            // A step that moved neither the stage nor a cursor and spent no probe cannot be repeated to any
            // effect, and looping on it would hang the game thread. That only happens when a stage rejected
            // its own inputs, which it has already diagnosed, so fail the build rather than spin.
            const auto StepMadeProgress =
                NOT (Get_StepFootprint(BuildStage, Scratch) == FootprintBeforeStep) || ProbesSpent > 0;

            if (NOT StepMadeProgress)
            {
                BuildStage = ECk_VoxelNav_BuildStage::Failed;
                break;
            }
        }

        Stats.Set_Slices(Stats.Get_Slices() + 1);
        Stats.Set_BuildSeconds(static_cast<float>(FPlatformTime::Seconds() - InOutState._StartSeconds));

        if (BuildStage != ECk_VoxelNav_BuildStage::Complete)
        { return; }

        voxelnav::Display(
            TEXT("VoxelNav bake complete in [{}] slices / [{}]s: [{}] occupancy probes, [{}] blocked layer-1 "
                 "cells, [{}] leaves ([{}] occluded), [{}] nodes total"),
            Stats.Get_Slices(),
            Stats.Get_BuildSeconds(),
            Stats.Get_OccupancyProbes(),
            Stats.Get_BlockedLayer1Cells(),
            Stats.Get_LeafNodeCount(),
            Stats.Get_OccludedLeafCount(),
            Stats.Get_TotalNodeCount());

        if (InParams._CellMerging != ECk_EnableDisable::Enable)
        { return; }

        voxelnav::Display(
            TEXT("VoxelNav cell merging: [{}] free cells merged into [{}] merged cells in [{}]s"),
            Stats.Get_PlainCellCount(),
            Stats.Get_MergedCellCount(),
            Stats.Get_MergeSeconds());
    }
}

// --------------------------------------------------------------------------------------------------------------------
