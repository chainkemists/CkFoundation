#include "CkGroundNav_Query_CellStep.h"

#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"
#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace cellstep_private
    {
        auto Get_IsWithinInterval(
            const FIntPoint& InCell,
            const FIntPoint& InMin,
            const FIntPoint& InMax) -> bool
        {
            return InCell.X >= InMin.X && InCell.X <= InMax.X &&
                   InCell.Y >= InMin.Y && InCell.Y <= InMax.Y;
        }

        // The plate a within-tile step lands on, or kNoPlate when no crossing covers this cell pair.
        // A portal is enumerated from its A side, so a step taken from the B side matches on the far
        // cell and the opposite direction.
        auto Get_PlateAcrossPortal(
            const FCk_GroundNav_Tile& InTile,
            const FIntPoint&          InFrom,
            const FIntPoint&          InTo,
            int32                     InPlate,
            int32                     InDirection) -> int32
        {
            for (const auto PortalIndex : InTile._Portals.Get_PortalsForPlate(InPlate))
            {
                const auto& Portal = InTile._Portals._Portals[PortalIndex];

                if (Portal._PlateA == InPlate && Portal._Direction == InDirection &&
                    Get_IsWithinInterval(InFrom, Portal._FromMin, Portal._FromMax))
                { return Portal._PlateB; }

                if (Portal._PlateB == InPlate && Get_OppositeDirection(Portal._Direction) == InDirection &&
                    Get_IsWithinInterval(InTo, Portal._FromMin, Portal._FromMax))
                { return Portal._PlateA; }
            }

            return FCk_GroundNav_Plate::kNoPlate;
        }

        // The tile and plate a seam step lands on, or kNoPlate. A seam runs along one axis and is
        // composed from its A side only, so a step out of the B side matches on the seam's B end.
        auto Get_PlateAcrossSeam(
            const FCk_GroundNav_Field& InField,
            int32                      InTileIndex,
            int32                      InPlate,
            const FIntPoint&           InFrom,
            int32                      InDirection,
            int32&                     OutTileIndex) -> int32
        {
            const auto Axis = InDirection % 2;
            const auto Along = Axis == 0 ? InFrom.Y : InFrom.X;
            const auto LeavesTowardPositive = InDirection < 2;

            for (const auto& Seam : InField._SeamPortals)
            {
                if (Seam._Direction != Axis || Along < Seam._AlongMin || Along > Seam._AlongMax)
                { continue; }

                if (LeavesTowardPositive && Seam._TileIndexA == InTileIndex && Seam._PlateA == InPlate)
                {
                    OutTileIndex = Seam._TileIndexB;
                    return Seam._PlateB;
                }

                if (NOT LeavesTowardPositive && Seam._TileIndexB == InTileIndex && Seam._PlateB == InPlate)
                {
                    OutTileIndex = Seam._TileIndexA;
                    return Seam._PlateA;
                }
            }

            return FCk_GroundNav_Plate::kNoPlate;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_StepAcross(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InFrom,
            int32                           InDirection,
            const FCk_GroundNav_QueryAgent& InAgent,
            FCk_GroundNav_SurfaceRef&       OutTo,
            float&                          OutSurfaceZUu,
            float&                          OutClearanceUu,
            FCk_GroundNav_QueryCost&        InOutCost)
        -> ECk_GroundNav_StepVerdict
    {
        using namespace cellstep_private;

        if (NOT InFrom.Get_IsValid() || NOT InField._Tiles.IsValidIndex(InFrom._TileIndex))
        { return ECk_GroundNav_StepVerdict::NoCrossing; }

        const auto& Tile = InField._Tiles[InFrom._TileIndex];
        const auto Offset = Get_DirectionOffset(InDirection);
        const auto From = FIntPoint{InFrom._CellX, InFrom._CellY};
        const auto To = From + Offset;

        auto TargetTileIndex = InFrom._TileIndex;
        auto TargetCell = To;
        auto TargetPlate = FCk_GroundNav_Plate::kNoPlate;

        const auto StaysInTile = To.X >= 0 && To.Y >= 0 && To.X < Tile._SizeX && To.Y < Tile._SizeY;

        if (StaysInTile)
        {
            ++InOutCost._CellsRead;

            TargetPlate = Tile._Plates.Get_PlateIndexAt(To.X, To.Y, InFrom._LayerIndex) == InFrom._PlateIndex
                ? InFrom._PlateIndex
                : Get_PlateAcrossPortal(Tile, From, To, InFrom._PlateIndex, InDirection);
        }
        else
        {
            const auto& Divisions = InField._Params._Divisions;
            const auto NeighbourCoord = FCk_GroundNav_TileCoord{Tile._Coord._X + Offset.X, Tile._Coord._Y + Offset.Y};

            if (NeighbourCoord._X < 0 || NeighbourCoord._Y < 0 ||
                NeighbourCoord._X >= Divisions.X || NeighbourCoord._Y >= Divisions.Y)
            { return ECk_GroundNav_StepVerdict::OutsideField; }

            const auto NeighbourIndex = Get_TileIndex(Divisions, NeighbourCoord);

            if (Get_TileStatus(InField, NeighbourIndex) != ECk_GroundNav_BuildStatus::Built)
            { return ECk_GroundNav_StepVerdict::Unbuilt; }

            TargetPlate = Get_PlateAcrossSeam(
                InField, InFrom._TileIndex, InFrom._PlateIndex, From, InDirection, TargetTileIndex);

            if (TargetPlate == FCk_GroundNav_Plate::kNoPlate)
            { return ECk_GroundNav_StepVerdict::NoCrossing; }

            // A seam that names some tile other than the lattice neighbour is a composition defect,
            // not a crossing; it must be seen, and it must not be walked.
            const auto SeamPointsAtTheNeighbour = TargetTileIndex == NeighbourIndex;
            CK_ENSURE_IF_NOT(SeamPointsAtTheNeighbour,
                TEXT("A seam portal out of tile [{}] plate [{}] names tile [{}], but the lattice neighbour is tile [{}]"),
                InFrom._TileIndex, InFrom._PlateIndex, TargetTileIndex, NeighbourIndex)
            { return ECk_GroundNav_StepVerdict::NoCrossing; }

            const auto& NeighbourTile = InField._Tiles[NeighbourIndex];

            TargetCell = FIntPoint{
                (To.X + NeighbourTile._SizeX) % NeighbourTile._SizeX,
                (To.Y + NeighbourTile._SizeY) % NeighbourTile._SizeY};
        }

        if (TargetPlate == FCk_GroundNav_Plate::kNoPlate)
        { return ECk_GroundNav_StepVerdict::NoCrossing; }

        const auto& TargetTile = InField._Tiles[TargetTileIndex];

        if (NOT TargetTile._Plates._Plates.IsValidIndex(TargetPlate))
        { return ECk_GroundNav_StepVerdict::NoCrossing; }

        auto Address = FCk_GroundNav_CellAddress{};
        Address._TileIndex = TargetTileIndex;
        Address._CellX = TargetCell.X;
        Address._CellY = TargetCell.Y;

        ++InOutCost._CellsRead;

        const auto TargetLayer = TargetTile._Plates._Plates[TargetPlate]._LayerIndex;

        if (NOT Get_SurfaceAt(InField, Address, TargetLayer, OutTo, OutSurfaceZUu, OutClearanceUu))
        { return ECk_GroundNav_StepVerdict::NoCrossing; }

        if (OutTo._PlateIndex != TargetPlate)
        { return ECk_GroundNav_StepVerdict::NoCrossing; }

        return Get_IsAdmitted(OutClearanceUu, InAgent)
            ? ECk_GroundNav_StepVerdict::Admitted
            : ECk_GroundNav_StepVerdict::Blocked;
    }
}

// --------------------------------------------------------------------------------------------------------------------
