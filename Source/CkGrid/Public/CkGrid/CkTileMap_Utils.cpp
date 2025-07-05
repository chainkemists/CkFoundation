#include "CkTileMap_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <PaperTileMap.h>
#include <PaperTileLayer.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TileMap_UE::
    Get_OccupancyInfo(
        const UPaperTileMap* InTileMap,
        int32 InLayer)
    -> FCk_TileMapOccupancyInfo
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Occupancy Info. TileMap is not valid!"))
    { return {}; }

    CK_ENSURE_IF_NOT(InTileMap->TileLayers.IsValidIndex(InLayer),
        TEXT("Failed to Get Occupancy Info. Invalid layer index [{}]"), InLayer)
    { return {}; }

    const auto& TileLayer = InTileMap->TileLayers[InLayer];

    CK_ENSURE_IF_NOT(ck::IsValid(TileLayer),
        TEXT("Failed to Get Occupancy Info. TileLayer [{}] of TileMap [{}] is not valid!"),
        InLayer,
        InTileMap)
    { return {}; }

    auto Result = FCk_TileMapOccupancyInfo{};
    Result.Set_TileMap(const_cast<UPaperTileMap*>(InTileMap));
    Result.Set_Layer(InLayer);
    Result.Set_Dimensions(FVector2D(InTileMap->MapWidth, InTileMap->MapHeight));

    auto OccupiedCoordinates = TArray<FIntPoint>{};
    auto UnoccupiedCoordinates = TArray<FIntPoint>{};

    for (auto X = 0; X < InTileMap->MapWidth; ++X)
    {
        for (auto Y = 0; Y < InTileMap->MapHeight; ++Y)
        {
            const auto& TileInfo = TileLayer->GetCell(X, Y);
            const auto Coordinate = FIntPoint(X, Y);

            if (TileInfo.IsValid())
            {
                OccupiedCoordinates.Add(Coordinate);
            }
            else
            {
                UnoccupiedCoordinates.Add(Coordinate);
            }
        }
    }

    Result.Set_OccupiedCoordinates(OccupiedCoordinates);
    Result.Set_UnoccupiedCoordinates(UnoccupiedCoordinates);

    return Result;
}

auto
    UCk_Utils_TileMap_UE::
    Get_MapSize(
        const UPaperTileMap* InTileMap,
        int32& OutMapWidth,
        int32& OutMapHeight,
        int32& OutNumLayers)
    -> void
{
    OutMapWidth = 0;
    OutMapHeight = 0;
    OutNumLayers = 0;

    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Map Size. InTileMap is not valid!"))
    { return; }

    OutMapWidth = InTileMap->MapWidth;
    OutMapHeight = InTileMap->MapHeight;
    OutNumLayers = InTileMap->TileLayers.Num();
}

auto
    UCk_Utils_TileMap_UE::
    Get_Tile(
        const UPaperTileMap* InTileMap,
        int32 InX,
        int32 InY,
        int32 InLayer)
    -> FPaperTileInfo
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Tile. InTileMap is not valid!"))
    { return {}; }

    CK_ENSURE_IF_NOT(Get_IsValidTilePosition(InTileMap, InX, InY, InLayer),
        TEXT("Failed to Get Tile. Invalid tile position [{}, {}, {}]"), InX, InY, InLayer)
    { return {}; }

    return InTileMap->TileLayers[InLayer]->GetCell(InX, InY);
}

auto
    UCk_Utils_TileMap_UE::
    Get_IsValidTilePosition(
        const UPaperTileMap* InTileMap,
        int32 InX,
        int32 InY,
        int32 InLayer)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Check Valid Tile Position. InTileMap is not valid!"))
    { return {}; }

    if (InX < 0 || InX >= InTileMap->MapWidth)
    { return {}; }

    if (InY < 0 || InY >= InTileMap->MapHeight)
    { return {}; }

    return InTileMap->TileLayers.IsValidIndex(InLayer);
}

auto
    UCk_Utils_TileMap_UE::
    Get_MapWidth(
        const UPaperTileMap* InTileMap)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Map Width. InTileMap is not valid!"))
    { return {}; }

    return InTileMap->MapWidth;
}

auto
    UCk_Utils_TileMap_UE::
    Get_MapHeight(
        const UPaperTileMap* InTileMap)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Map Height. InTileMap is not valid!"))
    { return {}; }

    return InTileMap->MapHeight;
}

auto
    UCk_Utils_TileMap_UE::
    Get_LayerCount(
        const UPaperTileMap* InTileMap)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InTileMap),
        TEXT("Failed to Get Layer Count. InTileMap is not valid!"))
    { return {}; }

    return InTileMap->TileLayers.Num();
}

// --------------------------------------------------------------------------------------------------------------------