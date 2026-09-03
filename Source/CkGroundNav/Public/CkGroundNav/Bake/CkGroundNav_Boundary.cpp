#include "CkGroundNav_Boundary.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace boundary_private
    {
        constexpr int32 kSideCount = 4;

        /**
         * The cells along one side of a plate, in the winding order (interior on the left of the
         * walk), and how many there are. Side 0 walks +Y up the east face, side 1 walks -X along the
         * north face, side 2 walks -Y down the west face, side 3 walks +X along the south face.
         */
        auto Get_SideCell(
            const FCk_GroundNav_Plate& InPlate,
            int32                      InSide,
            int32                      InStep) -> FIntPoint
        {
            switch (InSide)
            {
                case 0:  return FIntPoint{InPlate._MaxX, InPlate._MinY + InStep};
                case 1:  return FIntPoint{InPlate._MaxX - InStep, InPlate._MaxY};
                case 2:  return FIntPoint{InPlate._MinX, InPlate._MaxY - InStep};
                default: return FIntPoint{InPlate._MinX + InStep, InPlate._MinY};
            }
        }

        auto Get_SideLength(
            const FCk_GroundNav_Plate& InPlate,
            int32                      InSide) -> int32
        {
            return (InSide == 0 || InSide == 2) ? InPlate.Get_Depth() : InPlate.Get_Width();
        }

        auto Get_IsOnTileRim(
            const FCk_GroundNav_Plate&           InPlate,
            const FCk_GroundNav_BoundaryLattice& InLattice,
            int32                                InSide) -> bool
        {
            switch (InSide)
            {
                case 0:  return InPlate._MaxX == InLattice._SizeX - 1;
                case 1:  return InPlate._MaxY == InLattice._SizeY - 1;
                case 2:  return InPlate._MinX == 0;
                default: return InPlate._MinY == 0;
            }
        }

        /**
         * Whether a portal crosses this side at this cell. A portal is enumerated from its A side in
         * a positive direction, so a plate on the B side is crossed on the opposite face, one cell
         * over.
         */
        auto Get_IsCoveredByPortal(
            const FCk_GroundNav_Portal& InPortal,
            int32                       InPlateIndex,
            int32                       InSide,
            const FIntPoint&            InCell) -> bool
        {
            const auto IsPositiveSide = InSide == 0 || InSide == 1;
            const auto Axis = InSide % 2;

            if (InPortal._Direction != Axis)
            { return false; }

            if (IsPositiveSide)
            {
                return InPortal._PlateA == InPlateIndex &&
                       InCell.X >= InPortal._FromMin.X && InCell.X <= InPortal._FromMax.X &&
                       InCell.Y >= InPortal._FromMin.Y && InCell.Y <= InPortal._FromMax.Y;
            }

            const auto Offset = Axis == 0 ? FIntPoint{1, 0} : FIntPoint{0, 1};
            const auto FarCell = InCell - Offset;

            return InPortal._PlateB == InPlateIndex &&
                   FarCell.X >= InPortal._FromMin.X && FarCell.X <= InPortal._FromMax.X &&
                   FarCell.Y >= InPortal._FromMin.Y && FarCell.Y <= InPortal._FromMax.Y;
        }

        auto Do_Index(
            FCk_GroundNav_BoundaryField& InOutBoundary,
            int32                        InSegmentIndex,
            const FIntPoint&             InFromCell,
            const FIntPoint&             InToCell) -> void
        {
            const auto MinX = FMath::Min(InFromCell.X, InToCell.X);
            const auto MaxX = FMath::Max(InFromCell.X, InToCell.X);
            const auto MinY = FMath::Min(InFromCell.Y, InToCell.Y);
            const auto MaxY = FMath::Max(InFromCell.Y, InToCell.Y);

            const auto MinBucket = InOutBoundary.Get_BucketCoord(MinX, MinY);
            const auto MaxBucket = InOutBoundary.Get_BucketCoord(MaxX, MaxY);

            for (auto BucketY = MinBucket.Y; BucketY <= MaxBucket.Y; ++BucketY)
            {
                for (auto BucketX = MinBucket.X; BucketX <= MaxBucket.X; ++BucketX)
                {
                    if (NOT InOutBoundary.Get_IsValidBucket(FIntPoint{BucketX, BucketY}))
                    { continue; }

                    InOutBoundary._Buckets[(BucketY * InOutBoundary._BucketsX) + BucketX].Add(InSegmentIndex);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_BoundaryField::
        Get_AllocatedSize() const
        -> SIZE_T
    {
        auto Bytes = SIZE_T{0};

        Bytes += _Segments.GetAllocatedSize();
        Bytes += _EdgeCandidates.GetAllocatedSize();
        Bytes += _Buckets.GetAllocatedSize();

        for (const auto& Bucket : _Buckets)
        { Bytes += Bucket.GetAllocatedSize(); }

        return Bytes;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_BoundarySegment(
            const FCk_GroundNav_BoundaryLattice& InLattice,
            int32                                InPlateIndex,
            int32                                InLayerIndex,
            int32                                InSide,
            const FIntPoint&                     InFromCell,
            const FIntPoint&                     InToCell)
        -> FCk_GroundNav_BoundarySegment
    {
        const auto Cell = static_cast<double>(InLattice._CellSizeUu);

        const auto Get_Corner = [&](int32 InX, int32 InY, float InZ) -> FVector
        {
            return FVector{
                InLattice._Origin.X + (static_cast<double>(InX) * Cell),
                InLattice._Origin.Y + (static_cast<double>(InY) * Cell),
                static_cast<double>(InZ)};
        };

        const auto FromZ = InLattice.Get_SurfaceZ(InFromCell.X, InFromCell.Y, InLayerIndex);
        const auto ToZ = InLattice.Get_SurfaceZ(InToCell.X, InToCell.Y, InLayerIndex);

        auto Segment = FCk_GroundNav_BoundarySegment{};

        Segment._PlateIndex = InPlateIndex;
        Segment._LayerIndex = InLayerIndex;
        Segment._Side = InSide;
        Segment._FromCell = InFromCell;
        Segment._ToCell = InToCell;

        // The corner the walk starts from and the corner it ends past, on the face's own cell line.
        switch (InSide)
        {
            case 0:
                Segment._Start = Get_Corner(InFromCell.X + 1, InFromCell.Y, FromZ);
                Segment._End = Get_Corner(InToCell.X + 1, InToCell.Y + 1, ToZ);
                break;

            case 1:
                Segment._Start = Get_Corner(InFromCell.X + 1, InFromCell.Y + 1, FromZ);
                Segment._End = Get_Corner(InToCell.X, InToCell.Y + 1, ToZ);
                break;

            case 2:
                Segment._Start = Get_Corner(InFromCell.X, InFromCell.Y + 1, FromZ);
                Segment._End = Get_Corner(InToCell.X, InToCell.Y, ToZ);
                break;

            default:
                Segment._Start = Get_Corner(InFromCell.X, InFromCell.Y, FromZ);
                Segment._End = Get_Corner(InToCell.X + 1, InToCell.Y, ToZ);
                break;
        }

        const auto Along = FVector2D{Segment._End.X - Segment._Start.X, Segment._End.Y - Segment._Start.Y};

        Segment._InwardNormalXY = FVector2D{-Along.Y, Along.X}.GetSafeNormal();

        return Segment;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoDerive_Boundary(
            const FCk_GroundNav_BoundaryLattice& InLattice,
            const FCk_GroundNav_PlateField&      InPlates,
            const FCk_GroundNav_PortalField&     InPortals,
            FCk_GroundNav_BoundaryField&         OutBoundary,
            int32&                               InOutProbes)
        -> void
    {
        using namespace boundary_private;

        OutBoundary = FCk_GroundNav_BoundaryField{};

        if (InLattice._SizeX <= 0 || InLattice._SizeY <= 0 || InLattice._SurfaceZ == nullptr)
        { return; }

        OutBoundary._BucketsX = FMath::DivideAndRoundUp(InLattice._SizeX, FCk_GroundNav_BoundaryField::kBucketCells);
        OutBoundary._BucketsY = FMath::DivideAndRoundUp(InLattice._SizeY, FCk_GroundNav_BoundaryField::kBucketCells);
        OutBoundary._Buckets.SetNum(OutBoundary._BucketsX * OutBoundary._BucketsY);

        for (auto PlateIndex = 0; PlateIndex < InPlates._Plates.Num(); ++PlateIndex)
        {
            const auto& Plate = InPlates._Plates[PlateIndex];
            const auto PortalIndices = InPortals.Get_PortalsForPlate(PlateIndex);

            for (auto Side = 0; Side < kSideCount; ++Side)
            {
                const auto OnRim = Get_IsOnTileRim(Plate, InLattice, Side);
                const auto Length = Get_SideLength(Plate, Side);

                auto RunStart = 0;
                auto InRun = false;

                const auto Do_CloseRun = [&](int32 InEndStepExclusive) -> void
                {
                    if (NOT InRun)
                    { return; }

                    InRun = false;

                    const auto Segment = Make_BoundarySegment(
                        InLattice, PlateIndex, Plate._LayerIndex, Side,
                        Get_SideCell(Plate, Side, RunStart),
                        Get_SideCell(Plate, Side, InEndStepExclusive - 1));

                    if (OnRim)
                    {
                        OutBoundary._EdgeCandidates.Add(Segment);
                        return;
                    }

                    const auto SegmentIndex = OutBoundary._Segments.Add(Segment);

                    Do_Index(OutBoundary, SegmentIndex, Segment._FromCell, Segment._ToCell);
                };

                for (auto Step = 0; Step < Length; ++Step)
                {
                    ++InOutProbes;

                    const auto Cell = Get_SideCell(Plate, Side, Step);

                    auto Covered = false;

                    for (const auto PortalIndex : PortalIndices)
                    {
                        if (Get_IsCoveredByPortal(InPortals._Portals[PortalIndex], PlateIndex, Side, Cell))
                        {
                            Covered = true;
                            break;
                        }
                    }

                    if (Covered)
                    {
                        Do_CloseRun(Step);
                        continue;
                    }

                    if (NOT InRun)
                    {
                        InRun = true;
                        RunStart = Step;
                    }
                }

                Do_CloseRun(Length);
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
