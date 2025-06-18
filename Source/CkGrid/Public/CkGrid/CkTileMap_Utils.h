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

UCLASS(NotBlueprintable)
class CKGRID_API UCk_Utils_TileMap_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TileMap_UE);

public:
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