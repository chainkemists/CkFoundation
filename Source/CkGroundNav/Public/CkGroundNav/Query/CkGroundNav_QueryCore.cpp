#include "CkGroundNav_QueryCore.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace querycore_private
    {
        auto Get_SurfaceZInSamePlate(
            const FCk_GroundNav_Tile& InTile,
            int32                     InX,
            int32                     InY,
            int32                     InLayer,
            int32                     InPlateIndex,
            float&                    OutZ) -> bool
        {
            if (NOT InTile.Get_IsValidCell(InX, InY, InLayer))
            { return false; }

            if (InTile._Plates.Get_PlateIndexAt(InX, InY, InLayer) != InPlateIndex)
            { return false; }

            OutZ = InTile.Get_SurfaceZAt(InX, InY, InLayer);

            return OutZ != FCk_GroundNav_Tile::kNoSurfaceZ;
        }

        // Rise per unit of run along one axis, from whichever same-plate neighbours exist.
        auto Get_SlopeAlong(
            const FCk_GroundNav_Tile& InTile,
            int32                     InX,
            int32                     InY,
            int32                     InLayer,
            int32                     InPlateIndex,
            float                     InCentreZ,
            const FIntPoint&          InStep) -> double
        {
            const auto Cell = static_cast<double>(InTile._CellSizeUu);

            auto PlusZ = 0.0f;
            auto MinusZ = 0.0f;

            const auto HasPlus = Get_SurfaceZInSamePlate(
                InTile, InX + InStep.X, InY + InStep.Y, InLayer, InPlateIndex, PlusZ);
            const auto HasMinus = Get_SurfaceZInSamePlate(
                InTile, InX - InStep.X, InY - InStep.Y, InLayer, InPlateIndex, MinusZ);

            if (HasPlus && HasMinus)
            { return (static_cast<double>(PlusZ) - static_cast<double>(MinusZ)) / (2.0 * Cell); }

            if (HasPlus)
            { return (static_cast<double>(PlusZ) - static_cast<double>(InCentreZ)) / Cell; }

            if (HasMinus)
            { return (static_cast<double>(InCentreZ) - static_cast<double>(MinusZ)) / Cell; }

            return 0.0;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CellsPerTile(
            const FCk_GroundNav_FieldParams& InParams)
        -> int32
    {
        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());

        if (CellSize <= 0.0)
        { return 0; }

        return FMath::RoundToInt32(InParams.Get_TileSpanUu() / CellSize);
    }

    auto
        Get_FieldCellAt(
            const FCk_GroundNav_FieldParams& InParams,
            const FVector2D&                 InWorldXY)
        -> FIntPoint
    {
        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());

        if (CellSize <= 0.0)
        { return FIntPoint{INDEX_NONE, INDEX_NONE}; }

        return FIntPoint{
            FMath::FloorToInt32((InWorldXY.X - InParams._OriginXY.X) / CellSize),
            FMath::FloorToInt32((InWorldXY.Y - InParams._OriginXY.Y) / CellSize)};
    }

    auto
        Get_CellAddress(
            const FCk_GroundNav_Field& InField,
            const FIntPoint&           InFieldCell)
        -> FCk_GroundNav_CellAddress
    {
        const auto CellsPerTile = Get_CellsPerTile(InField._Params);

        if (CellsPerTile <= 0 || InFieldCell.X < 0 || InFieldCell.Y < 0)
        { return {}; }

        const auto Coord = FCk_GroundNav_TileCoord{InFieldCell.X / CellsPerTile, InFieldCell.Y / CellsPerTile};

        if (Coord._X >= InField._Params._Divisions.X || Coord._Y >= InField._Params._Divisions.Y)
        { return {}; }

        const auto TileIndex = Get_TileIndex(InField._Params._Divisions, Coord);

        if (NOT InField._Tiles.IsValidIndex(TileIndex))
        { return {}; }

        auto Address = FCk_GroundNav_CellAddress{};
        Address._TileIndex = TileIndex;
        Address._CellX = InFieldCell.X % CellsPerTile;
        Address._CellY = InFieldCell.Y % CellsPerTile;

        return Address;
    }

    auto
        Get_CellAddressAt(
            const FCk_GroundNav_Field& InField,
            const FVector2D&           InWorldXY)
        -> FCk_GroundNav_CellAddress
    {
        return Get_CellAddress(InField, Get_FieldCellAt(InField._Params, InWorldXY));
    }

    auto
        Get_CellAddressesAt(
            const FCk_GroundNav_Field&                              InField,
            const FVector2D&                                        InWorldXY,
            TArray<FCk_GroundNav_CellAddress, TInlineAllocator<4>>& OutCells)
        -> void
    {
        OutCells.Reset();

        const auto FloorCell = Get_FieldCellAt(InField._Params, InWorldXY);
        const auto Min = Get_CellMinXY(InField._Params, FloorCell);

        const auto OnLineX = InWorldXY.X == Min.X;
        const auto OnLineY = InWorldXY.Y == Min.Y;

        const auto Do_Add = [&](const FIntPoint& InCell) -> void
        {
            const auto Address = Get_CellAddress(InField, InCell);

            if (Address.Get_IsValid())
            { OutCells.Add(Address); }
        };

        Do_Add(FloorCell);

        if (OnLineX)
        { Do_Add(FIntPoint{FloorCell.X - 1, FloorCell.Y}); }

        if (OnLineY)
        { Do_Add(FIntPoint{FloorCell.X, FloorCell.Y - 1}); }

        if (OnLineX && OnLineY)
        { Do_Add(FIntPoint{FloorCell.X - 1, FloorCell.Y - 1}); }
    }

    auto
        Get_CellMinXY(
            const FCk_GroundNav_FieldParams& InParams,
            const FIntPoint&                 InFieldCell)
        -> FVector2D
    {
        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());

        return FVector2D{
            InParams._OriginXY.X + (static_cast<double>(InFieldCell.X) * CellSize),
            InParams._OriginXY.Y + (static_cast<double>(InFieldCell.Y) * CellSize)};
    }

    auto
        Get_ClosestPointInCellXY(
            const FCk_GroundNav_FieldParams& InParams,
            const FIntPoint&                 InFieldCell,
            const FVector2D&                 InWorldXY)
        -> FVector2D
    {
        const auto CellSize = static_cast<double>(InParams._Config.Get_CellSizeUu());
        const auto Min = Get_CellMinXY(InParams, InFieldCell);

        return FVector2D{
            FMath::Clamp(InWorldXY.X, Min.X, Min.X + CellSize),
            FMath::Clamp(InWorldXY.Y, Min.Y, Min.Y + CellSize)};
    }

    auto
        Get_HorizontalDistanceToCell(
            const FCk_GroundNav_FieldParams& InParams,
            const FIntPoint&                 InFieldCell,
            const FVector2D&                 InWorldXY)
        -> double
    {
        return FVector2D::Distance(InWorldXY, Get_ClosestPointInCellXY(InParams, InFieldCell, InWorldXY));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_TileStatus(
            const FCk_GroundNav_Field& InField,
            int32                      InTileIndex)
        -> ECk_GroundNav_BuildStatus
    {
        if (NOT InField._Tiles.IsValidIndex(InTileIndex))
        { return ECk_GroundNav_BuildStatus::Unbuilt; }

        return InField._Tiles[InTileIndex]._Status;
    }

    auto
        Get_SurfaceAt(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_CellAddress& InCell,
            int32                           InLayer,
            FCk_GroundNav_SurfaceRef&       OutSurface,
            float&                          OutSurfaceZUu,
            float&                          OutClearanceUu)
        -> bool
    {
        if (NOT InCell.Get_IsValid() || NOT InField._Tiles.IsValidIndex(InCell._TileIndex))
        { return false; }

        const auto& Tile = InField._Tiles[InCell._TileIndex];

        if (NOT Tile.Get_IsBuilt())
        { return false; }

        if (NOT Tile.Get_IsValidCell(InCell._CellX, InCell._CellY, InLayer))
        { return false; }

        const auto SurfaceZ = Tile.Get_SurfaceZAt(InCell._CellX, InCell._CellY, InLayer);

        if (SurfaceZ == FCk_GroundNav_Tile::kNoSurfaceZ)
        { return false; }

        const auto PlateIndex = Tile._Plates.Get_PlateIndexAt(InCell._CellX, InCell._CellY, InLayer);

        if (PlateIndex == FCk_GroundNav_Plate::kNoPlate)
        { return false; }

        OutSurface = FCk_GroundNav_SurfaceRef{};
        OutSurface._TileIndex = InCell._TileIndex;
        OutSurface._LayerIndex = InLayer;
        OutSurface._CellX = InCell._CellX;
        OutSurface._CellY = InCell._CellY;
        OutSurface._PlateIndex = PlateIndex;

        OutSurfaceZUu = SurfaceZ;
        OutClearanceUu = Tile._Clearance.Get_ClearanceAt(InCell._CellX, InCell._CellY, InLayer);

        return true;
    }

    auto
        Get_SurfaceNormal(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InSurface)
        -> FVector
    {
        using namespace querycore_private;

        if (NOT InSurface.Get_IsValid() || NOT InField._Tiles.IsValidIndex(InSurface._TileIndex))
        { return FVector::UpVector; }

        const auto& Tile = InField._Tiles[InSurface._TileIndex];

        if (NOT Tile.Get_IsValidCell(InSurface._CellX, InSurface._CellY, InSurface._LayerIndex) ||
            Tile._CellSizeUu <= 0.0f)
        { return FVector::UpVector; }

        const auto CentreZ = Tile.Get_SurfaceZAt(InSurface._CellX, InSurface._CellY, InSurface._LayerIndex);

        if (CentreZ == FCk_GroundNav_Tile::kNoSurfaceZ)
        { return FVector::UpVector; }

        const auto SlopeX = Get_SlopeAlong(Tile, InSurface._CellX, InSurface._CellY, InSurface._LayerIndex,
            InSurface._PlateIndex, CentreZ, FIntPoint{1, 0});
        const auto SlopeY = Get_SlopeAlong(Tile, InSurface._CellX, InSurface._CellY, InSurface._LayerIndex,
            InSurface._PlateIndex, CentreZ, FIntPoint{0, 1});

        return FVector{-SlopeX, -SlopeY, 1.0}.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
    }

    auto
        Get_SurfaceCentre(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_SurfaceRef& InSurface)
        -> FVector
    {
        if (NOT InSurface.Get_IsValid() || NOT InField._Tiles.IsValidIndex(InSurface._TileIndex))
        { return FVector::ZeroVector; }

        return InField._Tiles[InSurface._TileIndex].Get_CellCentre(
            InSurface._CellX, InSurface._CellY, InSurface._LayerIndex);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsAdmitted(
            float                          InClearanceUu,
            const FCk_GroundNav_QueryAgent& InAgent)
        -> bool
    {
        return InAgent._RadiusUu <= 0.0f || InClearanceUu >= InAgent._RadiusUu;
    }

    auto
        Get_IsRadiusAnswerable(
            const FCk_GroundNav_Field&      InField,
            const FCk_GroundNav_QueryAgent& InAgent)
        -> bool
    {
        return InAgent._RadiusUu <= InField._Params._MaxClearanceUu;
    }
}

// --------------------------------------------------------------------------------------------------------------------
