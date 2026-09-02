#include "CkGroundNav_Query_BuildStatus.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace buildstatus_private
    {
        // Inclusive on the faces, and false for a box that was never authored: a volume with no bounds
        // covers nothing, and the point and box forms must agree on where the edge is.
        auto Get_ContainsXY(
            const FBox&    InVolumeBounds,
            const FVector& InLocation) -> bool
        {
            if (NOT InVolumeBounds.IsValid)
            { return false; }

            return InLocation.X >= InVolumeBounds.Min.X && InLocation.X <= InVolumeBounds.Max.X &&
                   InLocation.Y >= InVolumeBounds.Min.Y && InLocation.Y <= InVolumeBounds.Max.Y;
        }

        auto Get_OverlapsXY(
            const FBox& InVolumeBounds,
            const FBox& InBounds) -> bool
        {
            if (NOT InVolumeBounds.IsValid || NOT InBounds.IsValid)
            { return false; }

            return InBounds.Max.X >= InVolumeBounds.Min.X && InBounds.Min.X <= InVolumeBounds.Max.X &&
                   InBounds.Max.Y >= InVolumeBounds.Min.Y && InBounds.Min.Y <= InVolumeBounds.Max.Y;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_RegionStatusAt(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation)
        -> ECk_GroundNav_RegionStatus
    {
        const auto Address = Get_CellAddressAt(InField, FVector2D{InLocation.X, InLocation.Y});

        if (NOT Address.Get_IsValid())
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        if (Get_TileStatus(InField, Address._TileIndex) == ECk_GroundNav_BuildStatus::Built)
        { return ECk_GroundNav_RegionStatus::Built; }

        return ECk_GroundNav_RegionStatus::Unbuilt;
    }

    auto
        Get_RegionStatusWithin(
            const FCk_GroundNav_Field& InField,
            const FBox&                InBounds)
        -> ECk_GroundNav_RegionStatus
    {
        const auto& Params = InField._Params;
        const auto CellsPerTile = Get_CellsPerTile(Params);

        if (CellsPerTile <= 0)
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        const auto LatticeMaxX = (CellsPerTile * Params._Divisions.X) - 1;
        const auto LatticeMaxY = (CellsPerTile * Params._Divisions.Y) - 1;

        if (LatticeMaxX < 0 || LatticeMaxY < 0)
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        const auto MinCell = Get_FieldCellAt(Params, FVector2D{InBounds.Min.X, InBounds.Min.Y});
        const auto MaxCell = Get_FieldCellAt(Params, FVector2D{InBounds.Max.X, InBounds.Max.Y});

        if (MaxCell.X < 0 || MaxCell.Y < 0 || MinCell.X > LatticeMaxX || MinCell.Y > LatticeMaxY)
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        const auto TileMinX = FMath::Max(MinCell.X, 0) / CellsPerTile;
        const auto TileMinY = FMath::Max(MinCell.Y, 0) / CellsPerTile;
        const auto TileMaxX = FMath::Min(MaxCell.X, LatticeMaxX) / CellsPerTile;
        const auto TileMaxY = FMath::Min(MaxCell.Y, LatticeMaxY) / CellsPerTile;

        auto TouchedCount = 0;
        auto BuiltCount = 0;

        for (auto TileY = TileMinY; TileY <= TileMaxY; ++TileY)
        {
            for (auto TileX = TileMinX; TileX <= TileMaxX; ++TileX)
            {
                const auto TileIndex = Get_TileIndex(Params._Divisions, FCk_GroundNav_TileCoord{TileX, TileY});

                ++TouchedCount;

                if (Get_TileStatus(InField, TileIndex) == ECk_GroundNav_BuildStatus::Built)
                { ++BuiltCount; }
            }
        }

        if (TouchedCount == 0)
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        if (BuiltCount == TouchedCount)
        { return ECk_GroundNav_RegionStatus::Built; }

        if (BuiltCount == 0)
        { return ECk_GroundNav_RegionStatus::Unbuilt; }

        return ECk_GroundNav_RegionStatus::PartiallyBuilt;
    }

    auto
        Get_SurfaceBounds(
            const FCk_GroundNav_Field& InField)
        -> FBox
    {
        auto Result = FBox{ForceInit};

        for (const auto& Tile : InField._Tiles)
        {
            if (NOT Tile.Get_IsBuilt())
            { continue; }

            const auto SpanX = static_cast<double>(Tile._SizeX) * static_cast<double>(Tile._CellSizeUu);
            const auto SpanY = static_cast<double>(Tile._SizeY) * static_cast<double>(Tile._CellSizeUu);

            Result += FBox{
                FVector{Tile._Origin.X, Tile._Origin.Y, static_cast<double>(InField._Params._MinZUu)},
                FVector{Tile._Origin.X + SpanX, Tile._Origin.Y + SpanY, static_cast<double>(InField._Params._MaxZUu)}};
        }

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_ProviderHealth(
            const FCk_GroundNav_FieldPtr& InPublished,
            ECk_GroundNav_BuildStatus     InLastBuildStatus,
            bool                          InIsBuildInFlight)
        -> ECk_NavSurface_ProviderHealth
    {
        if (InIsBuildInFlight)
        { return ECk_NavSurface_ProviderHealth::Building; }

        if (InPublished.IsValid())
        { return ECk_NavSurface_ProviderHealth::Ready; }

        if (InLastBuildStatus == ECk_GroundNav_BuildStatus::Failed)
        { return ECk_NavSurface_ProviderHealth::Error; }

        return ECk_NavSurface_ProviderHealth::NoData;
    }

    auto
        Get_RegionStatusAt_ForVolume(
            const FCk_GroundNav_FieldPtr& InPublished,
            const FBox&                   InVolumeBounds,
            bool                          InIsBuildInFlight,
            const FVector&                InLocation)
        -> ECk_GroundNav_RegionStatus
    {
        if (InPublished.IsValid())
        {
            const auto Status = Get_RegionStatusAt(*InPublished, InLocation);

            return Status == ECk_GroundNav_RegionStatus::Unbuilt && InIsBuildInFlight
                ? ECk_GroundNav_RegionStatus::Building
                : Status;
        }

        if (NOT buildstatus_private::Get_ContainsXY(InVolumeBounds, InLocation))
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        return InIsBuildInFlight
            ? ECk_GroundNav_RegionStatus::Building
            : ECk_GroundNav_RegionStatus::Unbuilt;
    }

    auto
        Get_RegionStatusWithin_ForVolume(
            const FCk_GroundNav_FieldPtr& InPublished,
            const FBox&                   InVolumeBounds,
            bool                          InIsBuildInFlight,
            const FBox&                   InBounds)
        -> ECk_GroundNav_RegionStatus
    {
        if (InPublished.IsValid())
        {
            const auto Status = Get_RegionStatusWithin(*InPublished, InBounds);

            const auto PromotesToBuilding =
                Status == ECk_GroundNav_RegionStatus::Unbuilt ||
                Status == ECk_GroundNav_RegionStatus::PartiallyBuilt;

            return PromotesToBuilding && InIsBuildInFlight
                ? ECk_GroundNav_RegionStatus::Building
                : Status;
        }

        if (NOT buildstatus_private::Get_OverlapsXY(InVolumeBounds, InBounds))
        { return ECk_GroundNav_RegionStatus::OutsideField; }

        return InIsBuildInFlight
            ? ECk_GroundNav_RegionStatus::Building
            : ECk_GroundNav_RegionStatus::Unbuilt;
    }
}

// --------------------------------------------------------------------------------------------------------------------
