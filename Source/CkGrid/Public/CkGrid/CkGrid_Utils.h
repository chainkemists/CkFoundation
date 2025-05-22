#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGrid_Utils.generated.h"

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
              Category="Ck|Utils|Grid2D")
    static bool
    Get_IsValidCoordinate(
        FIntPoint InGridDimensions,
        FIntPoint InCoordinate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Location -> Coordinate (Local)",
              Category="Ck|Utils|Grid2D")
    static FIntPoint
    Get_LocationAsCoordinate(
        FVector2D InLocation,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate -> Location (Local)",
              Category="Ck|Utils|Grid2D")
    static FVector2D
    Get_CoordinateAsLocation(
        FIntPoint InCoordinate,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate",
              Category="Ck|Utils|Grid2D")
    static FIntPoint
    TransformCoordinate(
        const FTransform& InTransform,
        FIntPoint InCoordinate,
        FVector2D InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location)",
              Category="Ck|Utils|Grid2D")
    static FVector2D
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        FIntPoint InCoordinate,
        FVector2D InCellSize);
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
              Category="Ck|Utils|Grid3D")
    static bool
    Get_IsValidCoordinate(
        FIntVector InGridDimensions,
        FIntVector InCoordinate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Location -> Coordinate (Local)",
              Category="Ck|Utils|Grid3D")
    static FIntVector
    Get_LocationAsCoordinate(
        FVector InLocation,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Coordinate -> Location (Local)",
              Category="Ck|Utils|Grid3D")
    static FVector
    Get_CoordinateAsLocation(
        FIntVector InCoordinate,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate",
              Category="Ck|Utils|Grid3D")
    static FIntVector
    TransformCoordinate(
        const FTransform& InTransform,
        FIntVector InCoordinate,
        FVector InCellSize);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][Grid] Transform Coordinate (As Location)",
              Category="Ck|Utils|Grid3D")
    static FVector
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        FIntVector InCoordinate,
        FVector InCellSize);
};

// --------------------------------------------------------------------------------------------------------------------
