#include "CkGroundNav_MeshClosure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace meshclosure_private
    {
        /**
         * Hands every welded position a stable id, treating two positions closer than the tolerance as
         * one vertex.
         *
         * The grid cell is the tolerance and a lookup probes all 27 cells around the query point rather
         * than the one it lands in. Quantizing straight to a cell key is the obvious alternative and is
         * wrong exactly at a boundary: two points a hair either side of one are within tolerance and
         * would still get different keys, so a shared corner that happened to straddle a grid line would
         * report a closed mesh as open — and the whole point of welding is that a backend may decode the
         * same vertex twice with different last bits.
         */
        class FWeldGrid
        {
        public:
            auto
            Get_Id(
                const FVector& InPosition) -> int32
            {
                const auto Cell = Get_Cell(InPosition);

                for (auto OffsetX = -1; OffsetX <= 1; ++OffsetX)
                {
                    for (auto OffsetY = -1; OffsetY <= 1; ++OffsetY)
                    {
                        for (auto OffsetZ = -1; OffsetZ <= 1; ++OffsetZ)
                        {
                            const auto* Bucket = _Buckets.Find(
                                FIntVector{Cell.X + OffsetX, Cell.Y + OffsetY, Cell.Z + OffsetZ});

                            if (Bucket == nullptr)
                            { continue; }

                            for (const auto Id : *Bucket)
                            {
                                if (FVector::DistSquared(_Positions[Id], InPosition) <= kToleranceSquared)
                                { return Id; }
                            }
                        }
                    }
                }

                const auto NewId = _Positions.Num();

                _Positions.Emplace(InPosition);
                _Buckets.FindOrAdd(Cell).Emplace(NewId);

                return NewId;
            }

            /** The FIRST position that welded to this id, so a reported edge is drawn where the mesh is. */
            auto
            Get_Position(
                int32 InId) const -> const FVector&
            {
                return _Positions[InId];
            }

        private:
            static auto
            Get_Cell(
                const FVector& InPosition) -> FIntVector
            {
                return FIntVector{
                    FMath::FloorToInt32(InPosition.X / kMeshClosureWeldToleranceUu),
                    FMath::FloorToInt32(InPosition.Y / kMeshClosureWeldToleranceUu),
                    FMath::FloorToInt32(InPosition.Z / kMeshClosureWeldToleranceUu)};
            }

        private:
            static constexpr auto kToleranceSquared =
                kMeshClosureWeldToleranceUu * kMeshClosureWeldToleranceUu;

            TArray<FVector> _Positions;

            TMap<FIntVector, TArray<int32>> _Buckets;
        };

        // ------------------------------------------------------------------------------------------------------------

        /**
         * One undirected edge as its two welded ids, smaller first in the high half.
         *
         * Both windings of a shared edge have to key alike or every edge of a closed mesh would be seen
         * once under two names, which is the same reading as a mesh with no interior at all.
         */
        auto Get_EdgeKey(
            int32 InLeft,
            int32 InRight) -> uint64
        {
            const auto Low = static_cast<uint64>(FMath::Min(InLeft, InRight));
            const auto High = static_cast<uint64>(FMath::Max(InLeft, InRight));

            return (Low << 32) | High;
        }

        auto Get_LowId(uint64 InKey) -> int32 { return static_cast<int32>(InKey >> 32); }

        auto Get_HighId(uint64 InKey) -> int32 { return static_cast<int32>(InKey & 0xFFFFFFFFULL); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MeshClosure(
            const FCk_GroundNav_GeometryBatch& InBatch,
            int32                              InFirstTriangle,
            int32                              InTriangleCount,
            int32                              InMaxRecordedEdges,
            int32&                             InOutProbes)
        -> FCk_GroundNav_ClosureReport
    {
        using namespace meshclosure_private;

        auto Report = FCk_GroundNav_ClosureReport{};

        const auto FirstTriangle = FMath::Max(0, InFirstTriangle);
        const auto LastTriangle = FMath::Min(
            InBatch.Get_TriangleCount(), FirstTriangle + FMath::Max(0, InTriangleCount));

        if (FirstTriangle >= LastTriangle)
        { return Report; }

        auto Weld = FWeldGrid{};

        auto EdgeCounts = TMap<uint64, int32>{};

        // Insertion order kept beside the map: the recorded edges are the first ones the mesh presented,
        // and a report that walked the map instead would depend on how the keys happened to hash.
        auto EdgeOrder = TArray<uint64>{};

        for (auto TriangleIndex = FirstTriangle; TriangleIndex < LastTriangle; ++TriangleIndex)
        {
            auto CornerA = FVector::ZeroVector;
            auto CornerB = FVector::ZeroVector;
            auto CornerC = FVector::ZeroVector;

            InBatch.Get_Triangle(TriangleIndex, CornerA, CornerB, CornerC);

            ++InOutProbes;

            const int32 Ids[] = {Weld.Get_Id(CornerA), Weld.Get_Id(CornerB), Weld.Get_Id(CornerC)};

            for (auto Corner = 0; Corner < 3; ++Corner)
            {
                const auto Left = Ids[Corner];
                const auto Right = Ids[(Corner + 1) % 3];

                // An edge whose ends welded together has zero length: it bounds nothing, and counting it
                // would read a sliver triangle as a hole in a mesh that is otherwise closed.
                if (Left == Right)
                { continue; }

                const auto Key = Get_EdgeKey(Left, Right);

                if (auto* Count = EdgeCounts.Find(Key))
                {
                    ++(*Count);
                    continue;
                }

                EdgeCounts.Add(Key, 1);
                EdgeOrder.Emplace(Key);
            }
        }

        Report._TriangleCount = LastTriangle - FirstTriangle;

        const auto MaxRecordedPoints = FMath::Max(0, InMaxRecordedEdges) * 2;

        for (const auto Key : EdgeOrder)
        {
            // An edge used three or more times is non-manifold rather than open, and is a different
            // defect with a different fix. This check answers only the question it was asked.
            if (EdgeCounts[Key] != 1)
            { continue; }

            ++Report._OpenEdgeCount;

            if (Report._OpenEdgePoints.Num() >= MaxRecordedPoints)
            { continue; }

            Report._OpenEdgePoints.Emplace(Weld.Get_Position(Get_LowId(Key)));
            Report._OpenEdgePoints.Emplace(Weld.Get_Position(Get_HighId(Key)));
        }

        return Report;
    }
}

// --------------------------------------------------------------------------------------------------------------------
