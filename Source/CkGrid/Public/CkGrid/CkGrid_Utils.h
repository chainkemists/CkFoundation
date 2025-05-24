#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGrid_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GridAnchor : uint8
{
    // Grid origins at (0, 0, 0) and scales +X and +Y
    Default,

    // Grid expands from the center of the grid
    Center UMETA(DisplayName = "Center Aligned"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GridAnchor);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGRID_API UCk_Utils_Grid2D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Grid2D_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Get Is Valid Coordinate",
              Category="Ck|Utils|Grid2D",
              meta = (Keywords = "tile, cell"))
    static bool
    Get_IsValidCoordinate(
        const FIntPoint& InGridDimensions,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Get Number Of Cells",
              Category="Ck|Utils|Grid2D",
              meta = (Keywords = "count, tile, size"))
    static int32
    Get_NumberOfCells(
        const FIntPoint& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate → Index (Local)",
              Category="Ck|Utils|Grid2D",
              meta = (Keywords = "cell, tile, flat"))
    static int32
    Get_CoordinateAsIndex(
        const FIntPoint& InCoordinate,
        const FIntPoint& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Index → Coordinate (Local)",
              Category="Ck|Utils|Grid2D",
              meta = (Keywords = "cell, tile, flat"))
    static FIntPoint
    Get_IndexAsCoordinate(
        int32 InIndex,
        const FIntPoint& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Location → Coordinate (Local)",
              Category="Ck|Utils|Grid2D")
    static FIntPoint
    Get_LocationAsCoordinate(
        FVector2D InLocation,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate → Location (Local)",
              Category="Ck|Utils|Grid2D")
    static FVector2D
    Get_CoordinateAsLocation(
        const FIntPoint& InCoordinate,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate",
              Category="Ck|Utils|Grid2D")
    static FIntPoint
    TransformCoordinate(
        const FTransform& InTransform,
        const FIntPoint& InCoordinate,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location)",
              Category="Ck|Utils|Grid2D")
    static FVector2D
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        const FIntPoint& InCoordinate,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location | Anchored)",
              Category="Ck|Utils|Grid2D")
    static FVector2D
    TransformCoordinate_AsLocation_Anchored(
        const FTransform& InTransform,
        const FIntPoint& InCoordinate,
        FVector2D InCellSize,
        const FIntPoint& InGridDimensions,
        ECk_GridAnchor InAnchor = ECk_GridAnchor::Default);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGRID_API UCk_Utils_Grid3D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Grid3D_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Get Is Valid Coordinate",
              Category="Ck|Utils|Grid3D",
              meta = (Keywords = "tile, cell"))
    static bool
    Get_IsValidCoordinate(
        const FIntVector& InGridDimensions,
        const FIntVector& InCoordinate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Get Number Of Cells",
              Category="Ck|Utils|Grid3D",
              meta = (Keywords = "count, tile, size"))
    static int32
    Get_NumberOfCells(
        const FIntVector& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate → Index (Local)",
              Category="Ck|Utils|Grid3D",
              meta = (Keywords = "cell, tile, flat"))
    static int32
    Get_CoordinateAsIndex(
        const FIntVector& InCoordinate,
        const FIntVector& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Index → Coordinate (Local)",
              Category="Ck|Utils|Grid3D",
              meta = (Keywords = "cell, tile, flat"))
    static FIntVector
    Get_IndexAsCoordinate(
        int32 InIndex,
        const FIntVector& InGridDimensions);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Location → Coordinate (Local)",
              Category="Ck|Utils|Grid3D")
    static FIntVector
    Get_LocationAsCoordinate(
        FVector InLocation,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate → Location (Local)",
              Category="Ck|Utils|Grid3D")
    static FVector
    Get_CoordinateAsLocation(
        const FIntVector& InCoordinate,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate",
              Category="Ck|Utils|Grid3D")
    static FIntVector
    TransformCoordinate(
        const FTransform& InTransform,
        const FIntVector& InCoordinate,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location)",
              Category="Ck|Utils|Grid3D")
    static FVector
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        const FIntVector& InCoordinate,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location | Anchored)",
              Category="Ck|Utils|Grid3D")
    static FVector
    TransformCoordinate_AsLocation_Anchored(
        const FTransform& InTransform,
        const FIntVector& InCoordinate,
        FVector InCellSize,
        const FIntVector& InGridDimensions,
        ECk_GridAnchor InAnchor = ECk_GridAnchor::Default);
};

// --------------------------------------------------------------------------------------------------------------------
