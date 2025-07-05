#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <Engine/Engine.h>

#include "CkTileMap_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UPaperTileMap;
class UPaperTileMapComponent;
struct FPaperTileInfo;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_TileMapOccupancyInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TileMapOccupancyInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    TObjectPtr<UPaperTileMap> _TileMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    int32 _Layer = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    FVector2D _Dimensions = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    TArray<FIntPoint> _OccupiedCoordinates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    TArray<FIntPoint> _UnoccupiedCoordinates;

public:
    CK_PROPERTY(_TileMap);
    CK_PROPERTY(_Layer);
    CK_PROPERTY(_Dimensions);
    CK_PROPERTY(_OccupiedCoordinates);
    CK_PROPERTY(_UnoccupiedCoordinates);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGRID_API UCk_Utils_TileMap_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TileMap_UE);

public:
    UFUNCTION(BlueprintCallable,
          Category = "Ck|Utils|TileMap",
          DisplayName = "[Ck] Get TileMap Occupancy Info",
          meta = (Keywords = "occupied, tiles, coordinates, dimensions"))
    static FCk_TileMapOccupancyInfo
    Get_OccupancyInfo(
        const UPaperTileMap* InTileMap,
        int32 InLayer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get TileMap Size",
              meta = (Keywords = "dimensions"))
    static void
    Get_MapSize(
        const UPaperTileMap* InTileMap,
        int32& OutMapWidth,
        int32& OutMapHeight,
        int32& OutNumLayers);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get Tile",
              meta = (Keywords = "cell"))
    static FPaperTileInfo
    Get_Tile(
        const UPaperTileMap* InTileMap,
        int32 InX,
        int32 InY,
        int32 InLayer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get Is Valid Tile Position",
              meta = (Keywords = "coordinate"))
    static bool
    Get_IsValidTilePosition(
        const UPaperTileMap* InTileMap,
        int32 InX,
        int32 InY,
        int32 InLayer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get TileMap Width")
    static int32
    Get_MapWidth(
        const UPaperTileMap* InTileMap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get TileMap Height")
    static int32
    Get_MapHeight(
        const UPaperTileMap* InTileMap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TileMap",
              DisplayName = "[Ck] Get TileMap Layer Count")
    static int32
    Get_LayerCount(
        const UPaperTileMap* InTileMap);
};

// --------------------------------------------------------------------------------------------------------------------