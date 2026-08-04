#include "CkVoxelNav_Octree_Merge.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_merge
{
    // Lattice coordinates are packed three-to-a-uint64, 20 bits each. The addressable cube is at most 2^15
    // finest cells across (14 layers, four sub-nodes per leaf edge), so a coordinate cannot reach the cap.
    constexpr auto CoordBits = 20;

    // The coarsest cell is 2^15 finest cells on a side, so every level index fits in the four bits a
    // geometry key has left over above its packed coordinate.
    constexpr auto LevelCount = 16;
    constexpr auto LevelShift = 60;

    // For each axis, the two axes that span the face pointing along it.
    constexpr int32 FaceAxes[3][2] = {{1, 2}, {2, 0}, {0, 1}};

    // Grow rows first, then slabs, then boxes - the classic greedy-meshing order, and the one that pairs
    // with the ascending scan order below (a seed is always the minimum corner of its unclaimed region).
    constexpr int32 GrowthAxes[3] = {0, 1, 2};

    /** One free octree cell as the merge pass sees it: a cube on the finest-cell lattice, its edge in
     *  lattice units, and the address it came from. Every octree cell is a cube aligned to its own size,
     *  which is what lets the growth pass find cells by an exact min-corner lookup and never search. */
    struct FLatticeCell
    {
        FIntVector _Min = FIntVector{0, 0, 0};
        int32 _Size = 0;
        int32 _Level = 0;
        ck::voxelnav::FNodeAddress _Address;
    };

    struct FCellLattice
    {
        TArray<FLatticeCell> _Cells;

        // Level -> packed min corner -> index into _Cells.
        TArray<TMap<uint64, int32>> _ByLevel;

        FVector _Origin = FVector::ZeroVector;
        double _UnitUu = 0.0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_LatticeKey(
            const FIntVector& InCoord)
        -> uint64
    {
        return (static_cast<uint64>(InCoord.X) << (CoordBits * 2)) |
               (static_cast<uint64>(InCoord.Y) << CoordBits) |
                static_cast<uint64>(InCoord.Z);
    }

    /** Identity of a cell as GEOMETRY rather than as an address: two octrees over the same lattice give the
     *  same cell the same key even though a rebuild moved its node index. That is what a local re-merge
     *  carries its untouched boxes across. */
    auto
        Get_GeometryKey(
            const FLatticeCell& InCell)
        -> uint64
    {
        return (static_cast<uint64>(InCell._Level) << LevelShift) | Get_LatticeKey(InCell._Min);
    }

    auto
        TryGet_CellAt(
            const FCellLattice& InLattice,
            const FIntVector& InMin,
            int32 InLevel)
        -> int32
    {
        if (NOT InLattice._ByLevel.IsValidIndex(InLevel))
        { return INDEX_NONE; }

        const auto* CellIndex = InLattice._ByLevel[InLevel].Find(Get_LatticeKey(InMin));

        return CellIndex != nullptr ? *CellIndex : INDEX_NONE;
    }

    auto
        TryGet_CellStartingAt(
            const FCellLattice& InLattice,
            const FIntVector& InMin)
        -> int32
    {
        for (auto Level = 0; Level < InLattice._ByLevel.Num(); ++Level)
        {
            const auto CellIndex = TryGet_CellAt(InLattice, InMin, Level);

            if (CellIndex != INDEX_NONE)
            { return CellIndex; }
        }

        return INDEX_NONE;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Build_Lattice(
            const ck::voxelnav::FOctree& InOctree,
            FCellLattice& OutLattice)
        -> void
    {
        using namespace ck::voxelnav;

        OutLattice._ByLevel.SetNum(LevelCount);

        const auto UnitUu = static_cast<double>(InOctree.Get_LeafNodes().Get_LeafSubNodeSize());

        if (UnitUu <= 0.0 || InOctree.Get_LayerCount() == 0)
        { return; }

        const auto& NavigationBounds = InOctree.Get_NavigationBounds();

        OutLattice._Origin = NavigationBounds.GetCenter() - NavigationBounds.GetExtent();
        OutLattice._UnitUu = UnitUu;

        auto Addresses = TArray<FNodeAddress>{};
        Get_FreeCells(InOctree, Addresses);

        OutLattice._Cells.Reserve(Addresses.Num());

        for (const auto& Address : Addresses)
        {
            const auto Bounds = Get_NodeBoundsFromAddress(InOctree, Address);

            if (Bounds.IsValid == 0)
            { continue; }

            const auto LocalMin = (Bounds.Min - OutLattice._Origin) / UnitUu;
            const auto Size = FMath::RoundToInt32((Bounds.Max.X - Bounds.Min.X) / UnitUu);

            if (Size <= 0)
            { continue; }

            const auto Level = static_cast<int32>(FMath::FloorLog2(static_cast<uint32>(Size)));

            if (Level >= LevelCount)
            { continue; }

            const auto Min = FIntVector
            {
                FMath::RoundToInt32(LocalMin.X),
                FMath::RoundToInt32(LocalMin.Y),
                FMath::RoundToInt32(LocalMin.Z)
            };

            const auto CellIndex = OutLattice._Cells.Emplace(FLatticeCell{Min, Size, Level, Address});

            OutLattice._ByLevel[Level].Add(Get_LatticeKey(Min), CellIndex);
        }
    }

    auto
        Get_WorldBounds(
            const FCellLattice& InLattice,
            const FIntVector& InBoxMin,
            const FIntVector& InBoxMax)
        -> FBox
    {
        const auto Min = InLattice._Origin + FVector{
            static_cast<double>(InBoxMin.X), static_cast<double>(InBoxMin.Y), static_cast<double>(InBoxMin.Z)} *
            InLattice._UnitUu;

        const auto Max = InLattice._Origin + FVector{
            static_cast<double>(InBoxMax.X), static_cast<double>(InBoxMax.Y), static_cast<double>(InBoxMax.Z)} *
            InLattice._UnitUu;

        return FBox{Min, Max};
    }

    // ----------------------------------------------------------------------------------------------------------------

    /** Extends the box by one slab along InAxis, if and only if that slab is tiled EXACTLY by unclaimed
     *  cells of one size. Whole cells only: a box that swallowed half of a cell would leave that cell with
     *  no merged cell to belong to, and the cell -> merged lookup is what resolves a search endpoint. */
    auto
        TryExtend_Box(
            const FCellLattice& InLattice,
            const TArray<int32>& InAssignment,
            FIntVector& InOutBoxMin,
            FIntVector& InOutBoxMax,
            int32 InAxis,
            TArray<int32>& OutAbsorbed)
        -> bool
    {
        OutAbsorbed.Reset();

        const auto FacePosition = InOutBoxMax[InAxis];

        auto ProbeCoord = InOutBoxMin;
        ProbeCoord[InAxis] = FacePosition;

        const auto FirstCellIndex = TryGet_CellStartingAt(InLattice, ProbeCoord);

        if (FirstCellIndex == INDEX_NONE || InAssignment[FirstCellIndex] != INDEX_NONE)
        { return false; }

        const auto SlabSize = InLattice._Cells[FirstCellIndex]._Size;
        const auto SlabLevel = InLattice._Cells[FirstCellIndex]._Level;

        const auto AxisU = FaceAxes[InAxis][0];
        const auto AxisV = FaceAxes[InAxis][1];

        // A cell of edge S sits on an S-aligned lattice position, so a face that is not S-aligned and a
        // whole number of S across cannot be tiled by them however the rest of the slab looks.
        const auto FaceTilesBySlabSize =
            InOutBoxMin[AxisU] % SlabSize == 0 &&
            InOutBoxMin[AxisV] % SlabSize == 0 &&
            (InOutBoxMax[AxisU] - InOutBoxMin[AxisU]) % SlabSize == 0 &&
            (InOutBoxMax[AxisV] - InOutBoxMin[AxisV]) % SlabSize == 0;

        if (NOT FaceTilesBySlabSize)
        { return false; }

        for (auto U = InOutBoxMin[AxisU]; U < InOutBoxMax[AxisU]; U += SlabSize)
        {
            for (auto V = InOutBoxMin[AxisV]; V < InOutBoxMax[AxisV]; V += SlabSize)
            {
                auto Coord = InOutBoxMin;

                Coord[InAxis] = FacePosition;
                Coord[AxisU] = U;
                Coord[AxisV] = V;

                const auto CellIndex = TryGet_CellAt(InLattice, Coord, SlabLevel);

                if (CellIndex == INDEX_NONE || InAssignment[CellIndex] != INDEX_NONE)
                { return false; }

                OutAbsorbed.Emplace(CellIndex);
            }
        }

        InOutBoxMax[InAxis] += SlabSize;

        return true;
    }

    /** Grows a merged box out of every cell the caller left unassigned, in a deterministic scan order.
     *  Cells arriving already assigned (a local re-merge's carried-over boxes) are blockers, so a
     *  re-derived box can never overlap one that was carried over. */
    auto
        Run_GreedyMerge(
            const FCellLattice& InLattice,
            TArray<int32>& InOutAssignment,
            ck::voxelnav::FMergedCellTable& InOutTable)
        -> void
    {
        auto MergedCellCounts = TArray<int32>{};
        MergedCellCounts.Init(0, InOutTable.Get_CellCount());

        for (const auto ExistingMergedIndex : InOutAssignment)
        {
            if (MergedCellCounts.IsValidIndex(ExistingMergedIndex))
            { ++MergedCellCounts[ExistingMergedIndex]; }
        }

        auto Order = TArray<int32>{};
        Order.Reserve(InLattice._Cells.Num());

        for (auto CellIndex = 0; CellIndex < InLattice._Cells.Num(); ++CellIndex)
        { Order.Emplace(CellIndex); }

        // Ascending lattice position, so the seed of a region is always its minimum corner and growing
        // along the positive axes alone reaches the whole region. The order is also what makes a merge
        // deterministic: the cell enumeration's own order is an octree traversal detail.
        Order.Sort([&InLattice](int32 InLeft, int32 InRight) -> bool
        {
            const auto& Left = InLattice._Cells[InLeft];
            const auto& Right = InLattice._Cells[InRight];

            if (Left._Min.Z != Right._Min.Z) { return Left._Min.Z < Right._Min.Z; }
            if (Left._Min.Y != Right._Min.Y) { return Left._Min.Y < Right._Min.Y; }
            if (Left._Min.X != Right._Min.X) { return Left._Min.X < Right._Min.X; }

            return Left._Size < Right._Size;
        });

        auto Absorbed = TArray<int32>{};

        for (const auto SeedIndex : Order)
        {
            if (InOutAssignment[SeedIndex] != INDEX_NONE)
            { continue; }

            const auto& Seed = InLattice._Cells[SeedIndex];

            auto BoxMin = Seed._Min;
            auto BoxMax = Seed._Min + FIntVector{Seed._Size, Seed._Size, Seed._Size};

            const auto MergedIndex = InOutTable.Request_AddCell(ck::voxelnav::FMergedCell{});

            InOutAssignment[SeedIndex] = MergedIndex;

            auto CellCount = 1;

            for (const auto Axis : GrowthAxes)
            {
                while (TryExtend_Box(InLattice, InOutAssignment, BoxMin, BoxMax, Axis, Absorbed))
                {
                    for (const auto AbsorbedIndex : Absorbed)
                    { InOutAssignment[AbsorbedIndex] = MergedIndex; }

                    CellCount += Absorbed.Num();
                }
            }

            MergedCellCounts.Emplace(CellCount);

            InOutTable.Request_SetBounds(MergedIndex, Get_WorldBounds(InLattice, BoxMin, BoxMax));
        }

        for (auto MergedIndex = 0; MergedIndex < MergedCellCounts.Num(); ++MergedIndex)
        { InOutTable.Request_SetCellCount(MergedIndex, MergedCellCounts[MergedIndex]); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    /** Face adjacency, read off the octree's OWN neighbour links: two merged cells are neighbours when a
     *  cell of one has a cell of the other as a face neighbour. Deriving it rather than computing box
     *  adjacency geometrically means the merged graph inherits every correctness property of the walk the
     *  plain graph already used. */
    auto
        Build_Adjacency(
            const ck::voxelnav::FOctree& InOctree,
            const FCellLattice& InLattice,
            const TArray<int32>& InAssignment,
            ck::voxelnav::FMergedCellTable& InOutTable)
        -> void
    {
        using namespace ck::voxelnav;

        auto NeighborSets = TArray<TSet<int32>>{};
        NeighborSets.SetNum(InOutTable.Get_CellCount());

        auto NeighbourAddresses = TArray<FNodeAddress>{};

        for (auto CellIndex = 0; CellIndex < InLattice._Cells.Num(); ++CellIndex)
        {
            const auto MergedIndex = InAssignment[CellIndex];

            if (NOT NeighborSets.IsValidIndex(MergedIndex))
            { continue; }

            NeighbourAddresses.Reset();
            Get_NodeNeighbours(InOctree, InLattice._Cells[CellIndex]._Address, NeighbourAddresses);

            for (const auto& NeighbourAddress : NeighbourAddresses)
            {
                const auto NeighbourMergedIndex = InOutTable.Get_MergedIndexForCell(NeighbourAddress);

                if (NOT NeighborSets.IsValidIndex(NeighbourMergedIndex) || NeighbourMergedIndex == MergedIndex)
                { continue; }

                // Both directions: a search relies on the graph being walkable back the way it came, and
                // the octree's own links are one-directional per face.
                NeighborSets[MergedIndex].Add(NeighbourMergedIndex);
                NeighborSets[NeighbourMergedIndex].Add(MergedIndex);
            }
        }

        for (auto MergedIndex = 0; MergedIndex < NeighborSets.Num(); ++MergedIndex)
        {
            auto Neighbors = NeighborSets[MergedIndex].Array();
            Neighbors.Sort();

            InOutTable.Request_SetNeighbors(MergedIndex, MoveTemp(Neighbors));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Get_FreeCells(
            const FOctree& InOctree,
            TArray<FNodeAddress>& OutCells)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Get_FreeCells);

        const auto LayerCount = InOctree.Get_LayerCount();

        if (LayerCount == 0)
        { return; }

        const auto TopLayerIndex = static_cast<LayerIndex>(LayerCount - 1);
        const auto& TopLayer = InOctree.Get_Layer(TopLayerIndex);

        for (auto NodeIdx = 0; NodeIdx < TopLayer.Get_NodeCount(); ++NodeIdx)
        { Get_FreeNodesUnderAddress(InOctree, FNodeAddress::Make(TopLayerIndex, NodeIdx), OutCells); }
    }

    auto
        Get_NodeBoundsFromAddress(
            const FOctree& InOctree,
            const FNodeAddress& InAddress)
        -> FBox
    {
        const auto Extent = Get_NodeExtentFromAddress(InOctree, InAddress);

        if (Extent <= 0.0f)
        { return FBox{ForceInit}; }

        const auto Center = Get_NodePositionFromAddress(
            InOctree, InAddress, ECk_VoxelNav_SubNodePrecision::SubNodeCenter);

        return FBox::BuildAABB(Center, FVector{Extent});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_MergeCells(
            FOctree& InOutOctree)
        -> void
    {
        using namespace ck_voxelnav_octree_merge;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Request_MergeCells);

        auto& Table = InOutOctree.Get_MergedCells();
        Table.Request_Reset();

        auto Lattice = FCellLattice{};
        Build_Lattice(InOutOctree, Lattice);

        auto Assignment = TArray<int32>{};
        Assignment.Init(INDEX_NONE, Lattice._Cells.Num());

        Run_GreedyMerge(Lattice, Assignment, Table);

        for (auto CellIndex = 0; CellIndex < Lattice._Cells.Num(); ++CellIndex)
        { Table.Request_MapCellToMerged(Lattice._Cells[CellIndex]._Address, Assignment[CellIndex]); }

        Build_Adjacency(InOutOctree, Lattice, Assignment, Table);

        constexpr auto NothingCarriedOver = 0;
        Table.Request_MarkMerged(Lattice._Cells.Num(), NothingCarriedOver);
    }

    auto
        Request_MergeCellsLocally(
            FOctree& InOutOctree,
            const FOctree& InPublishedOctree,
            const FBox& InDirtyBounds)
        -> void
    {
        using namespace ck_voxelnav_octree_merge;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Request_MergeCellsLocally);

        const auto& PublishedTable = InPublishedOctree.Get_MergedCells();

        // Two octrees over different lattices share no cell geometry, so there is nothing a carry-over
        // could key on - and a table that was never merged has nothing to carry over in the first place.
        const auto CanCarryOver =
            PublishedTable.Get_IsMerged() &&
            InDirtyBounds.IsValid != 0 &&
            InPublishedOctree.Get_NavigationBounds() == InOutOctree.Get_NavigationBounds() &&
            InPublishedOctree.Get_LeafNodes().Get_LeafNodeSize() ==
                InOutOctree.Get_LeafNodes().Get_LeafNodeSize();

        if (NOT CanCarryOver)
        {
            Request_MergeCells(InOutOctree);
            return;
        }

        auto& Table = InOutOctree.Get_MergedCells();
        Table.Request_Reset();

        auto PublishedToCarriedOver = TArray<int32>{};
        PublishedToCarriedOver.Init(INDEX_NONE, PublishedTable.Get_CellCount());

        for (auto PublishedIndex = 0; PublishedIndex < PublishedTable.Get_CellCount(); ++PublishedIndex)
        {
            const auto& PublishedCell = PublishedTable.Get_Cells()[PublishedIndex];

            if (PublishedCell.Get_Bounds().Intersect(InDirtyBounds))
            { continue; }

            PublishedToCarriedOver[PublishedIndex] = Table.Request_AddCell(FMergedCell{PublishedCell.Get_Bounds()});
        }

        const auto CarriedOverCount = Table.Get_CellCount();

        auto PublishedLattice = FCellLattice{};
        Build_Lattice(InPublishedOctree, PublishedLattice);

        auto CarriedOverByGeometry = TMap<uint64, int32>{};
        CarriedOverByGeometry.Reserve(PublishedLattice._Cells.Num());

        for (const auto& PublishedCell : PublishedLattice._Cells)
        {
            const auto PublishedMergedIndex = PublishedTable.Get_MergedIndexForCell(PublishedCell._Address);

            if (NOT PublishedToCarriedOver.IsValidIndex(PublishedMergedIndex) ||
                PublishedToCarriedOver[PublishedMergedIndex] == INDEX_NONE)
            { continue; }

            CarriedOverByGeometry.Add(
                Get_GeometryKey(PublishedCell), PublishedToCarriedOver[PublishedMergedIndex]);
        }

        auto Lattice = FCellLattice{};
        Build_Lattice(InOutOctree, Lattice);

        auto Assignment = TArray<int32>{};
        Assignment.Init(INDEX_NONE, Lattice._Cells.Num());

        for (auto CellIndex = 0; CellIndex < Lattice._Cells.Num(); ++CellIndex)
        {
            const auto* CarriedOverIndex = CarriedOverByGeometry.Find(Get_GeometryKey(Lattice._Cells[CellIndex]));

            if (CarriedOverIndex == nullptr)
            { continue; }

            Assignment[CellIndex] = *CarriedOverIndex;
        }

        Run_GreedyMerge(Lattice, Assignment, Table);

        for (auto CellIndex = 0; CellIndex < Lattice._Cells.Num(); ++CellIndex)
        { Table.Request_MapCellToMerged(Lattice._Cells[CellIndex]._Address, Assignment[CellIndex]); }

        Build_Adjacency(InOutOctree, Lattice, Assignment, Table);

        Table.Request_MarkMerged(Lattice._Cells.Num(), CarriedOverCount);
    }
}

// --------------------------------------------------------------------------------------------------------------------
