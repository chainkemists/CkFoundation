#include "CkGrid/CkGrid_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Grid2D_UE::
    Get_IsValidCoordinate(
        FIntPoint InGridDimensions,
        FIntPoint InCoordinate)
    -> bool
{
    const auto IsValid = InCoordinate.X >= 0 && InCoordinate.X < InGridDimensions.X &&
                         InCoordinate.Y >= 0 && InCoordinate.Y < InGridDimensions.Y;
    return IsValid;
}

auto
    UCk_Utils_Grid2D_UE::
    Get_LocationAsCoordinate(
        FVector2D InLocation,
        FVector2D InCellSize)
    -> FIntPoint
{
    const auto Coordinate = FIntPoint(
        FMath::FloorToInt(InLocation.X / InCellSize.X),
        FMath::FloorToInt(InLocation.Y / InCellSize.Y));
    return Coordinate;
}

auto
    UCk_Utils_Grid2D_UE::
    Get_CoordinateAsLocation(
        FIntPoint InCoordinate,
        FVector2D InCellSize)
    -> FVector2D
{
    const auto Location = FVector2D(
        (InCoordinate.X * InCellSize.X),
        (InCoordinate.Y * InCellSize.Y));
    return Location;
}

auto
    UCk_Utils_Grid2D_UE::
    TransformCoordinate(
        const FTransform& InTransform,
        FIntPoint InCoordinate,
        FVector2D InCellSize)
    -> FIntPoint
{
    const auto LocalLocation = Get_CoordinateAsLocation(InCoordinate, InCellSize);

    const auto LocalLocation3D = FVector(LocalLocation.X, LocalLocation.Y, 0.0f);
    const auto WorldLocation3D = InTransform.TransformPosition(LocalLocation3D);

    const auto WorldLocation2D = FVector2D(WorldLocation3D.X, WorldLocation3D.Y);
    const auto WorldCoordinate = Get_LocationAsCoordinate(WorldLocation2D, InCellSize);

    return WorldCoordinate;
}

auto
    UCk_Utils_Grid2D_UE::
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        FIntPoint InCoordinate,
        FVector2D InCellSize)
    -> FVector2D
{
    const auto CellScale = FTransform{FRotator::ZeroRotator, FVector::ZeroVector, FVector(InCellSize.X, InCellSize.Y, 1.0f)};
    const auto ScaledCoordinate = CellScale.TransformPosition(FVector(InCoordinate.X, InCoordinate.Y, 0.0f));
    const auto WorldLocation = InTransform.TransformPosition(ScaledCoordinate);

    return FVector2D(WorldLocation.X, WorldLocation.Y);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Grid3D_UE::
    Get_IsValidCoordinate(
        FIntVector InGridDimensions,
        FIntVector InCoordinate)
    -> bool
{
    const auto IsValid = InCoordinate.X >= 0 && InCoordinate.X < InGridDimensions.X &&
                         InCoordinate.Y >= 0 && InCoordinate.Y < InGridDimensions.Y &&
                         InCoordinate.Z >= 0 && InCoordinate.Z < InGridDimensions.Z;
    return IsValid;
}

auto
    UCk_Utils_Grid3D_UE::
    Get_LocationAsCoordinate(
        FVector InLocation,
        FVector InCellSize)
    -> FIntVector
{
    const auto Coordinate = FIntVector(
        FMath::FloorToInt(InLocation.X / InCellSize.X),
        FMath::FloorToInt(InLocation.Y / InCellSize.Y),
        FMath::FloorToInt(InLocation.Z / InCellSize.Z));
    return Coordinate;
}

auto
    UCk_Utils_Grid3D_UE::
    Get_CoordinateAsLocation(
        FIntVector InCoordinate,
        FVector InCellSize)
    -> FVector
{
    const auto Location = FVector(
        (InCoordinate.X * InCellSize.X) ,
        (InCoordinate.Y * InCellSize.Y),
        (InCoordinate.Z * InCellSize.Z));
    return Location;
}

auto
    UCk_Utils_Grid3D_UE::
    TransformCoordinate(
        const FTransform& InTransform,
        FIntVector InCoordinate,
        FVector InCellSize)
    -> FIntVector
{
    const auto LocalLocation = Get_CoordinateAsLocation(InCoordinate, InCellSize);
    const auto WorldLocation = InTransform.TransformPosition(LocalLocation);
    const auto WorldCoordinate = Get_LocationAsCoordinate(WorldLocation, InCellSize);

    return WorldCoordinate;
}

auto
    UCk_Utils_Grid3D_UE::
    TransformCoordinate_AsLocation(
        const FTransform& InTransform,
        FIntVector InCoordinate,
        FVector InCellSize)
    -> FVector
{
    const auto CellScale = FTransform{FRotator::ZeroRotator, FVector::ZeroVector, InCellSize};
    const auto ScaledCoordinate = CellScale.TransformPosition(FVector(InCoordinate.X, InCoordinate.Y, InCoordinate.Z));
    const auto WorldLocation = InTransform.TransformPosition(ScaledCoordinate);

    return WorldLocation;
}

// --------------------------------------------------------------------------------------------------------------------
