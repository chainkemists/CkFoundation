#include "CkGroundNav_FieldTypes.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_TileIndex(
            const FIntPoint&               InDivisions,
            const FCk_GroundNav_TileCoord& InCoord)
        -> int32
    {
        const auto IsInside = InCoord._X >= 0 && InCoord._Y >= 0 &&
                              InCoord._X < InDivisions.X && InCoord._Y < InDivisions.Y;

        return IsInside ? (InCoord._Y * InDivisions.X) + InCoord._X : INDEX_NONE;
    }

    auto
        Get_TileCoord(
            const FIntPoint& InDivisions,
            int32            InTileIndex)
        -> FCk_GroundNav_TileCoord
    {
        if (InDivisions.X <= 0 || InTileIndex < 0)
        { return FCk_GroundNav_TileCoord{}; }

        return FCk_GroundNav_TileCoord{InTileIndex % InDivisions.X, InTileIndex / InDivisions.X};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_Tile::
        Get_AllocatedSize() const
        -> SIZE_T
    {
        auto Bytes = SIZE_T{0};

        Bytes += _SurfaceZ.GetAllocatedSize();

        Bytes += _Clearance._Cells.GetAllocatedSize();

        Bytes += _Plates._Plates.GetAllocatedSize();
        Bytes += _Plates._CellToPlate.GetAllocatedSize();

        Bytes += _Portals._Portals.GetAllocatedSize();
        Bytes += _Portals._PlateToPortals.GetAllocatedSize();

        for (const auto& PlatePortals : _Portals._PlateToPortals)
        { Bytes += PlatePortals.GetAllocatedSize(); }

        Bytes += _SeamStubs.GetAllocatedSize();

        Bytes += _Boundary.Get_AllocatedSize();

        return Bytes;
    }

    auto
        FCk_GroundNav_Tile::
        Get_CellCentre(
            int32 InX,
            int32 InY,
            int32 InLayer) const
        -> FVector
    {
        const auto HalfCell = static_cast<double>(_CellSizeUu) * 0.5;

        return FVector{
            _Origin.X + (static_cast<double>(InX) * _CellSizeUu) + HalfCell,
            _Origin.Y + (static_cast<double>(InY) * _CellSizeUu) + HalfCell,
            static_cast<double>(Get_SurfaceZAt(InX, InY, InLayer))};
    }

    auto
        FCk_GroundNav_Tile::
        Get_WalkableCellCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& SurfaceZ : _SurfaceZ)
        {
            if (SurfaceZ != kNoSurfaceZ)
            { ++Count; }
        }

        return Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------
