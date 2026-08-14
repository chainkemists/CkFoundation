#include "CkVoxelNav_Octree_Repair.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkVoxelNav/CkVoxelNav_Log.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Merge.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Morton.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

#include <HAL/PlatformTime.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_repair
{
    constexpr auto WorkingStageCount = static_cast<int32>(ECk_VoxelNav_RepairStage::Complete);

    constexpr auto ChildrenPerNode = 8;
    constexpr auto SubNodesPerLeaf = 64;

    constexpr auto LeafLayerIndex = ck::voxelnav::LayerIndex{0};
    constexpr auto LeafParentLayerIndex = ck::voxelnav::LayerIndex{1};

    constexpr auto FirstInteriorLayerIndex = 1;

    auto
        Get_StageSpendsProbes(
            ECk_VoxelNav_RepairStage InStage)
        -> bool
    {
        return InStage == ECk_VoxelNav_RepairStage::ProbeDirtyCells ||
               InStage == ECk_VoxelNav_RepairStage::ProbeDirtyLeaves;
    }

    struct FStepFootprint
    {
        ECk_VoxelNav_RepairStage _Stage = ECk_VoxelNav_RepairStage::NotStarted;
        int32 _CellCursor = 0;
        int32 _LeafCursor = 0;
        int32 _SubCursor = 0;
        int32 _LayerCursor = 0;

        auto operator==(const FStepFootprint&) const -> bool = default;
    };

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
        Get_CellMortonsIntersectingBounds(
            const FOctree& InOctree,
            LayerIndex InLayerIndex,
            const FBox& InBounds,
            TArray<MortonCode>& OutMortonCodes)
        -> void
    {
        OutMortonCodes.Reset();

        const auto LayerIsInRange = InLayerIndex < InOctree.Get_LayerCount();

        CK_ENSURE_IF_NOT(LayerIsInRange,
            TEXT("Layer [{}] is outside the VoxelNav octree's [{}] layers"),
            static_cast<int32>(InLayerIndex), InOctree.Get_LayerCount())
        { return; }

        if (InBounds.IsValid == 0)
        { return; }

        const auto& Layer = InOctree.Get_Layer(InLayerIndex);
        const auto CellSize = static_cast<double>(Layer.Get_NodeSize());
        const auto EdgeCellCount = Layer.Get_EdgeNodeCount();

        const auto CellsAreSized = CellSize > 0.0 && EdgeCellCount > 0;

        CK_ENSURE_IF_NOT(CellsAreSized,
            TEXT("VoxelNav layer [{}] has a cell size of [{}]uu across [{}] cells per axis and cannot be "
                 "addressed by bounds"),
            static_cast<int32>(InLayerIndex), CellSize, EdgeCellCount)
        { return; }

        const auto& NavigationBounds = InOctree.Get_NavigationBounds();
        const auto LatticeOrigin = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();

        const auto LocalMin = InBounds.Min - LatticeOrigin;
        const auto LocalMax = InBounds.Max - LatticeOrigin;

        const auto Get_ClampedCoord = [&](double InLocal) -> int32
        {
            return FMath::Clamp(
                static_cast<int32>(FMath::FloorToDouble(InLocal / CellSize)), 0, EdgeCellCount - 1);
        };

        // Rejected BEFORE clamping: a box entirely off one side of the lattice clamps onto the lattice's
        // edge row and would otherwise dirty cells it never touched.
        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            if (LocalMax[Axis] < 0.0 || LocalMin[Axis] >= static_cast<double>(EdgeCellCount) * CellSize)
            { return; }
        }

        const auto MinCoords = FIntVector
        {
            Get_ClampedCoord(LocalMin.X), Get_ClampedCoord(LocalMin.Y), Get_ClampedCoord(LocalMin.Z)
        };

        const auto MaxCoords = FIntVector
        {
            Get_ClampedCoord(LocalMax.X), Get_ClampedCoord(LocalMax.Y), Get_ClampedCoord(LocalMax.Z)
        };

        OutMortonCodes.Reserve(
            (MaxCoords.X - MinCoords.X + 1) * (MaxCoords.Y - MinCoords.Y + 1) * (MaxCoords.Z - MinCoords.Z + 1));

        for (auto CoordZ = MinCoords.Z; CoordZ <= MaxCoords.Z; ++CoordZ)
        {
            for (auto CoordY = MinCoords.Y; CoordY <= MaxCoords.Y; ++CoordY)
            {
                for (auto CoordX = MinCoords.X; CoordX <= MaxCoords.X; ++CoordX)
                { OutMortonCodes.Emplace(Get_MortonFromCoords(FIntVector{CoordX, CoordY, CoordZ})); }
            }
        }

        // Ascending Morton order makes the probe sequence a property of the dirty region alone, so a repair
        // costs and produces the same thing however the region was reached.
        OutMortonCodes.Sort();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FRepairState::
        Get_Progress01() const
        -> float
    {
        using namespace ck_voxelnav_octree_repair;

        if (_Stage == ECk_VoxelNav_RepairStage::Complete)
        { return 1.0f; }

        if (_Stage == ECk_VoxelNav_RepairStage::Failed)
        { return 0.0f; }

        return static_cast<float>(static_cast<int32>(_Stage)) / static_cast<float>(WorkingStageCount);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_ResetRepair(
            FRepairState& InOutState)
        -> void
    {
        InOutState._Stage = ECk_VoxelNav_RepairStage::NotStarted;
        InOutState._Stats = FCk_VoxelNav_RepairStats{};
        InOutState._StartSeconds = 0.0;

        InOutState._PublishedOctree.Reset();

        Request_ResetOctree(InOutState._Octree);
        Request_ResetScratch(InOutState._Scratch);

        InOutState._LeafOccupancy.Reset();
        InOutState._DirtyCellMortons.Reset();
        InOutState._DirtyLeafMortons.Reset();

        InOutState._CellCursor = 0;
        InOutState._LeafCursor = 0;
        InOutState._SubCursor = 0;
        InOutState._PendingLeafWord = 0;
    }

    auto
        Request_FailRepair(
            FRepairState& InOutState)
        -> void
    {
        const auto Stats = InOutState._Stats;

        Request_ResetRepair(InOutState);

        InOutState._Stats = Stats;
        InOutState._Stage = ECk_VoxelNav_RepairStage::Failed;
    }

    auto
        Request_BeginRepair(
            FRepairState& InOutState,
            TSharedPtr<const FOctree> InPublishedOctree)
        -> bool
    {
        Request_ResetRepair(InOutState);

        const auto PublishedOctreeIsBuilt = InPublishedOctree.IsValid() && InPublishedOctree->Get_IsValid();

        CK_ENSURE_IF_NOT(PublishedOctreeIsBuilt,
            TEXT("Cannot repair a VoxelNav octree that was never published - a local repair carries the "
                 "occupancy of everything OUTSIDE its dirty bounds over from the previous bake, and there is "
                 "none to carry"))
        {
            InOutState._Stage = ECk_VoxelNav_RepairStage::Failed;
            return false;
        }

        InOutState._PublishedOctree = MoveTemp(InPublishedOctree);

        return true;
    }

    auto
        Request_ReleaseRepairedOctree(
            FRepairState& InOutState)
        -> TSharedPtr<const FOctree>
    {
        if (InOutState._Stage != ECk_VoxelNav_RepairStage::Complete)
        { return {}; }

        auto Published = MakeShared<FOctree>(MoveTemp(InOutState._Octree));

        const auto Stats = InOutState._Stats;

        Request_ResetRepair(InOutState);

        InOutState._Stats = Stats;
        InOutState._Stage = ECk_VoxelNav_RepairStage::Complete;

        return Published;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_AdvanceRepair(
            FRepairState& InOutState,
            const FRepairParams& InParams,
            const ICk_VoxelNav_GeometryBackend& InBackend,
            const FBuildBudget& InBudget)
        -> void
    {
        using namespace ck_voxelnav_octree_repair;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Request_AdvanceRepair);

        if (InOutState.Get_IsFinished())
        { return; }

        if (InOutState._Stage == ECk_VoxelNav_RepairStage::NotStarted)
        { InOutState._StartSeconds = FPlatformTime::Seconds(); }

        // Bound once here rather than reached through the state below, so the stage machine reads as plain
        // operations on a stage, an octree, a scratch, an occupancy record and a set of cursors.
        auto& RepairStage = InOutState._Stage;
        auto& WorkingOctree = InOutState._Octree;
        auto& Scratch = InOutState._Scratch;
        auto& Stats = InOutState._Stats;
        auto& LeafOccupancy = InOutState._LeafOccupancy;
        auto& DirtyCellMortons = InOutState._DirtyCellMortons;
        auto& DirtyLeafMortons = InOutState._DirtyLeafMortons;
        auto& CellCursor = InOutState._CellCursor;
        auto& LeafCursor = InOutState._LeafCursor;
        auto& SubCursor = InOutState._SubCursor;
        auto& PendingLeafWord = InOutState._PendingLeafWord;
        const auto& PublishedOctreePtr = InOutState._PublishedOctree;

        // Clearance inflates every probe box, so a cell whose probe box reaches the dirty region can change
        // even when the cell itself does not touch it. Expanding once here is what keeps the dirty set a
        // superset of what actually changed.
        const auto ExpandedDirtyBounds = InParams._DirtyBounds.IsValid != 0
            ? InParams._DirtyBounds.ExpandBy(static_cast<double>(InParams._ClearanceUu))
            : InParams._DirtyBounds;

        // The merge half of the local-repair contract: the published table's boxes clear of the dirty region
        // are carried over, and only what the obstacle reached is merged again. Expanded bounds, for the
        // same reason the probes use them - a cell whose probe box reached the dirty region can have changed.
        const auto DoMergeCells = [&]() -> void
        {
            if (InParams._CellMerging != ECk_EnableDisable::Enable)
            { return; }

            const auto MergeStartSeconds = FPlatformTime::Seconds();

            if (PublishedOctreePtr.IsValid())
            { Request_MergeCellsLocally(WorkingOctree, *PublishedOctreePtr, ExpandedDirtyBounds); }
            else
            { Request_MergeCells(WorkingOctree); }

            const auto& MergedCells = WorkingOctree.Get_MergedCells();

            Stats.Set_MergedCellCount(MergedCells.Get_CellCount());
            Stats.Set_MergedCellsCarriedOver(MergedCells.Get_CarriedOverCount());
            Stats.Set_MergeSeconds(static_cast<float>(FPlatformTime::Seconds() - MergeStartSeconds));
        };

        const auto DoFinishRepair = [&]() -> void
        {
            DoMergeCells();

            Request_MarkOctreeBuilt(WorkingOctree);

            Stats.Set_LeafNodeCount(WorkingOctree.Get_Layer(0).Get_NodeCount());
            Stats.Set_TotalNodeCount(Get_TotalNodeCount(WorkingOctree));

            RepairStage = ECk_VoxelNav_RepairStage::Complete;
        };

        // A leaf's recorded occupancy is REPLACED, never accumulated into: an obstacle that vacated a leaf
        // has to be able to clear the bits it set, and a word of zero means "no entry" so the map only ever
        // holds leaves that actually hold geometry.
        const auto DoRecordLeafWord = [&](MortonCode InLeafMorton, uint64 InWord) -> void
        {
            const auto* ExistingWord = LeafOccupancy.Find(InLeafMorton);
            const auto PreviousWord = ExistingWord != nullptr ? *ExistingWord : 0ULL;

            if (PreviousWord != InWord)
            { Stats.Set_LeavesChanged(Stats.Get_LeavesChanged() + 1); }

            if (InWord == 0)
            {
                LeafOccupancy.Remove(InLeafMorton);
                return;
            }

            LeafOccupancy.Add(InLeafMorton, InWord);
        };

        const auto DoSeedFromPublished = [&]() -> bool
        {
            const auto RepairWasOpened = PublishedOctreePtr.IsValid();

            CK_ENSURE_IF_NOT(RepairWasOpened,
                TEXT("A VoxelNav repair was advanced without being opened against a published octree"))
            { return false; }

            const auto OctreeInitialized = Request_InitializeOctree(
                WorkingOctree, InParams._FinestCellSizeUu, InParams._VolumeBounds);

            if (NOT OctreeInitialized)
            { return false; }

            Request_InitializeScratch(WorkingOctree, Scratch);

            const auto& PublishedOctree = *PublishedOctreePtr;

            const auto LatticesMatch =
                WorkingOctree.Get_LayerCount() == PublishedOctree.Get_LayerCount() &&
                Scratch._BlockedNodes.Num() > 0;

            CK_ENSURE_IF_NOT(LatticesMatch,
                TEXT("A VoxelNav repair sized to [{}] layers cannot carry occupancy over from a bake of [{}] "
                     "layers - the repair params must be the ones the volume was baked with"),
                WorkingOctree.Get_LayerCount(), PublishedOctree.Get_LayerCount())
            { return false; }

            const auto& PublishedLeafLayer = PublishedOctree.Get_Layer(LeafLayerIndex);
            const auto& PublishedLeaves = PublishedOctree.Get_LeafNodes().Get_LeafNodes();

            LeafOccupancy.Reserve(PublishedLeafLayer.Get_NodeCount());

            for (const auto& PublishedNode : PublishedLeafLayer.Get_Nodes())
            {
                Scratch._BlockedNodes[0].Add(Get_ParentMorton(PublishedNode.Get_MortonCode()));

                if (NOT PublishedNode.Get_HasChildren())
                { continue; }

                const auto PublishedLeafIndex = PublishedNode.Get_FirstChild().Get_NodeIndex();

                if (NOT PublishedLeaves.IsValidIndex(PublishedLeafIndex))
                { continue; }

                const auto PublishedWord = PublishedLeaves[PublishedLeafIndex].Get_SubNodes();

                if (PublishedWord == 0)
                { continue; }

                LeafOccupancy.Add(PublishedNode.Get_MortonCode(), PublishedWord);
            }

            Get_CellMortonsIntersectingBounds(
                WorkingOctree, LeafParentLayerIndex, ExpandedDirtyBounds, DirtyCellMortons);

            CellCursor = 0;

            return true;
        };

        const auto DoCollectDirtyLeaves = [&]() -> void
        {
            auto LeavesInBounds = TArray<MortonCode>{};
            Get_CellMortonsIntersectingBounds(
                WorkingOctree, LeafLayerIndex, ExpandedDirtyBounds, LeavesInBounds);

            DirtyLeafMortons.Reset();
            DirtyLeafMortons.Reserve(LeavesInBounds.Num());

            // A leaf only exists under a blocked layer-1 parent. One under an unblocked parent holds no
            // geometry by definition, so probing it would spend budget to learn what the cell probe above it
            // already established.
            for (const auto LeafMorton : LeavesInBounds)
            {
                if (NOT Scratch._BlockedNodes[0].Contains(Get_ParentMorton(LeafMorton)))
                { continue; }

                DirtyLeafMortons.Emplace(LeafMorton);
            }

            LeafCursor = 0;
            SubCursor = 0;
            PendingLeafWord = 0;
        };

        const auto DoProbeDirtyCells = [&](int32 InProbeBudget) -> int32
        {
            QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Repair_ProbeDirtyCells);

            auto ProbesSpent = 0;

            const auto CellCount = DirtyCellMortons.Num();
            const auto ProbeHalfExtents = FVector
            {
                WorkingOctree.Get_Layer(LeafParentLayerIndex).Get_NodeExtent() + InParams._ClearanceUu
            };

            for (; CellCursor < CellCount && ProbesSpent < InProbeBudget; ++CellCursor)
            {
                const auto CellMorton = DirtyCellMortons[CellCursor];
                const auto CellPosition = Get_NodePositionFromLayerAndMorton(
                    WorkingOctree, LeafParentLayerIndex, CellMorton);

                ++ProbesSpent;
                Stats.Set_DirtyCellsProbed(Stats.Get_DirtyCellsProbed() + 1);

                const auto CellIsOccupied = InBackend.Get_IsBoxOccupied(CellPosition, ProbeHalfExtents);
                const auto CellWasBlocked = Scratch._BlockedNodes[0].Contains(CellMorton);

                if (CellIsOccupied)
                {
                    Scratch._BlockedNodes[0].Add(CellMorton);
                    continue;
                }

                if (NOT CellWasBlocked)
                { continue; }

                Scratch._BlockedNodes[0].Remove(CellMorton);

                // The cell holds no geometry at all, so neither do its eight leaves. Dropping their recorded
                // occupancy is what frees the space an obstacle left behind - keeping it is how a moving
                // obstacle smears a permanent trail through the navigation data.
                const auto FirstChildMorton = Get_FirstChildMorton(CellMorton);

                for (auto ChildOffset = 0; ChildOffset < ChildrenPerNode; ++ChildOffset)
                { DoRecordLeafWord(FirstChildMorton + ChildOffset, 0); }
            }

            return ProbesSpent;
        };

        const auto DoProbeDirtyLeaves = [&](int32 InProbeBudget) -> int32
        {
            QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Repair_ProbeDirtyLeaves);

            auto ProbesSpent = 0;

            const auto& LeafNodes = WorkingOctree.Get_LeafNodes();

            const auto LeafCount = DirtyLeafMortons.Num();
            const auto LeafNodeExtent = LeafNodes.Get_LeafNodeExtent();
            const auto SubNodeSize = LeafNodes.Get_LeafSubNodeSize();
            const auto SubNodeExtent = LeafNodes.Get_LeafSubNodeExtent();

            const auto LeafProbeHalfExtents = FVector{LeafNodeExtent + InParams._ClearanceUu};
            const auto SubNodeProbeHalfExtents = FVector{SubNodeExtent + InParams._ClearanceUu};

            while (LeafCursor < LeafCount && ProbesSpent < InProbeBudget)
            {
                const auto LeafMorton = DirtyLeafMortons[LeafCursor];
                const auto LeafPosition = Get_LeafNodePositionFromMorton(WorkingOctree, LeafMorton);

                if (SubCursor == 0)
                {
                    ++ProbesSpent;
                    Stats.Set_DirtyLeavesProbed(Stats.Get_DirtyLeavesProbed() + 1);

                    if (NOT InBackend.Get_IsBoxOccupied(LeafPosition, LeafProbeHalfExtents))
                    {
                        DoRecordLeafWord(LeafMorton, 0);
                        ++LeafCursor;
                        continue;
                    }

                    PendingLeafWord = 0;
                    SubCursor = 1;
                    continue;
                }

                const auto LeafOrigin = LeafPosition - FVector{LeafNodeExtent};

                for (; SubCursor <= SubNodesPerLeaf && ProbesSpent < InProbeBudget;
                       ++SubCursor)
                {
                    const auto SubNodeIdx = static_cast<SubNodeIndex>(SubCursor - 1);
                    const auto SubNodeCenter = LeafOrigin +
                                               Get_VectorFromMorton(SubNodeIdx) * SubNodeSize +
                                               FVector{SubNodeExtent};

                    ++ProbesSpent;

                    if (NOT InBackend.Get_IsBoxOccupied(SubNodeCenter, SubNodeProbeHalfExtents))
                    { continue; }

                    PendingLeafWord |= 1ULL << SubNodeIdx;
                }

                if (SubCursor <= SubNodesPerLeaf)
                { break; }

                DoRecordLeafWord(LeafMorton, PendingLeafWord);

                ++LeafCursor;
                SubCursor = 0;
                PendingLeafWord = 0;
            }

            return ProbesSpent;
        };

        const auto DoApplyOccupancy = [&]() -> void
        {
            auto& LeafNodes = WorkingOctree.Get_LeafNodes();

            for (auto LeafIdx = 0; LeafIdx < Scratch._LeafMortonCodes.Num(); ++LeafIdx)
            {
                const auto* RecordedWord = LeafOccupancy.Find(Scratch._LeafMortonCodes[LeafIdx]);

                LeafNodes.Get_LeafNode(LeafIdx).Set_SubNodes(
                    RecordedWord != nullptr ? *RecordedWord : 0ULL);
            }
        };

        const auto DoAdvanceOneStep = [&](int32 InProbeBudget) -> int32
        {
            switch (RepairStage)
            {
                case ECk_VoxelNav_RepairStage::NotStarted:
                {
                    RepairStage = ECk_VoxelNav_RepairStage::SeedFromPublished;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::SeedFromPublished:
                {
                    RepairStage = DoSeedFromPublished()
                        ? ECk_VoxelNav_RepairStage::ProbeDirtyCells
                        : ECk_VoxelNav_RepairStage::Failed;

                    return 0;
                }
                case ECk_VoxelNav_RepairStage::ProbeDirtyCells:
                {
                    const auto ProbesSpent = DoProbeDirtyCells(InProbeBudget);

                    if (CellCursor >= DirtyCellMortons.Num())
                    {
                        DoCollectDirtyLeaves();
                        RepairStage = ECk_VoxelNav_RepairStage::ProbeDirtyLeaves;
                    }

                    return ProbesSpent;
                }
                case ECk_VoxelNav_RepairStage::ProbeDirtyLeaves:
                {
                    const auto ProbesSpent = DoProbeDirtyLeaves(InProbeBudget);

                    if (LeafCursor >= DirtyLeafMortons.Num())
                    { RepairStage = ECk_VoxelNav_RepairStage::PropagateBlocked; }

                    return ProbesSpent;
                }
                case ECk_VoxelNav_RepairStage::PropagateBlocked:
                {
                    Stage_PropagateBlockedUpward(WorkingOctree, Scratch);

                    RepairStage = ECk_VoxelNav_RepairStage::AllocateLeaves;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::AllocateLeaves:
                {
                    Stage_AllocateLeaves(WorkingOctree, Scratch);

                    // The sort stage decides which layer-0 nodes get a child link from this set, so it has to
                    // hold exactly the leaves the repair recorded occupancy for.
                    Scratch._OccludedLeafMortons.Reset();

                    for (const auto& RecordedLeaf : LeafOccupancy)
                    { Scratch._OccludedLeafMortons.Add(RecordedLeaf.Key); }

                    RepairStage = ECk_VoxelNav_RepairStage::SortLeafLayer;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::SortLeafLayer:
                {
                    Stage_SortLeafLayer(WorkingOctree, Scratch);

                    RepairStage = ECk_VoxelNav_RepairStage::ApplyOccupancy;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::ApplyOccupancy:
                {
                    DoApplyOccupancy();

                    Scratch._LayerCursor = FirstInteriorLayerIndex;
                    RepairStage = ECk_VoxelNav_RepairStage::RasterizeLayers;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::RasterizeLayers:
                {
                    if (Scratch._LayerCursor >= WorkingOctree.Get_LayerCount())
                    {
                        RepairStage = ECk_VoxelNav_RepairStage::BuildParentLinks;
                        return 0;
                    }

                    Stage_RasterizeLayer(WorkingOctree, Scratch,
                        static_cast<LayerIndex>(Scratch._LayerCursor));

                    ++Scratch._LayerCursor;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::BuildParentLinks:
                {
                    Stage_BuildParentLinks(WorkingOctree, Scratch);

                    Scratch._LayerCursor = WorkingOctree.Get_LayerCount() - 2;
                    RepairStage = ECk_VoxelNav_RepairStage::BuildNeighbourLinks;
                    return 0;
                }
                case ECk_VoxelNav_RepairStage::BuildNeighbourLinks:
                {
                    if (Scratch._LayerCursor < 0)
                    {
                        DoFinishRepair();
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

        const auto Get_StepFootprint = [&]() -> FStepFootprint
        {
            return FStepFootprint
            {
                RepairStage,
                CellCursor,
                LeafCursor,
                SubCursor,
                Scratch._LayerCursor
            };
        };

        auto ProbesRemaining = InBudget._MaxOccupancyProbes;
        const auto SliceStartSeconds = FPlatformTime::Seconds();

        for (;;)
        {
            if (Get_StageSpendsProbes(RepairStage) && ProbesRemaining <= 0)
            { break; }

            if (InBudget._MaxSeconds > 0.0f &&
                FPlatformTime::Seconds() - SliceStartSeconds >= static_cast<double>(InBudget._MaxSeconds))
            { break; }

            const auto FootprintBeforeStep = Get_StepFootprint();
            const auto ProbesSpent = DoAdvanceOneStep(ProbesRemaining);

            ProbesRemaining -= ProbesSpent;
            Stats.Set_OccupancyProbes(Stats.Get_OccupancyProbes() + ProbesSpent);

            if (InOutState.Get_IsFinished())
            { break; }

            // Same guard the bake carries: a step that moved no cursor, changed no stage and spent no probe
            // cannot be repeated to any effect, so failing beats spinning the game thread.
            const auto StepMadeProgress =
                NOT (Get_StepFootprint() == FootprintBeforeStep) || ProbesSpent > 0;

            if (NOT StepMadeProgress)
            {
                RepairStage = ECk_VoxelNav_RepairStage::Failed;
                break;
            }
        }

        Stats.Set_Slices(Stats.Get_Slices() + 1);
        Stats.Set_RepairSeconds(static_cast<float>(FPlatformTime::Seconds() - InOutState._StartSeconds));

        if (RepairStage != ECk_VoxelNav_RepairStage::Complete)
        { return; }

        voxelnav::Verbose(
            TEXT("VoxelNav local repair complete in [{}] slices / [{}]s: [{}] probes over [{}] dirty cells and "
                 "[{}] dirty leaves, [{}] leaves changed, [{}] leaves and [{}] nodes total"),
            Stats.Get_Slices(),
            Stats.Get_RepairSeconds(),
            Stats.Get_OccupancyProbes(),
            Stats.Get_DirtyCellsProbed(),
            Stats.Get_DirtyLeavesProbed(),
            Stats.Get_LeavesChanged(),
            Stats.Get_LeafNodeCount(),
            Stats.Get_TotalNodeCount());
    }
}

// --------------------------------------------------------------------------------------------------------------------
