#include "CkGrid/CkGrid_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Grid2D_UE::
    Get_IsValidCoordinate(
        const FIntPoint& InGridDimensions,
        const FIntPoint& InCoordinate)
    -> bool
{
    const auto IsValid = InCoordinate.X >= 0 && InCoordinate.X < InGridDimensions.X &&
                         InCoordinate.Y >= 0 && InCoordinate.Y < InGridDimensions.Y;
    return IsValid;
}

auto
    UCk_Utils_Grid2D_UE::
    Get_NumberOfCells(
        const FIntPoint& InGridDimensions)
    -> int32
{
    return InGridDimensions.X * InGridDimensions.Y;
}

auto
    UCk_Utils_Grid2D_UE::
    Get_CoordinateAsIndex(
        const FIntPoint& InCoordinate,
        const FIntPoint& InGridDimensions)
    -> int32
{
    CK_ENSURE_IF_NOT(Get_IsValidCoordinate(InGridDimensions, InCoordinate),
        TEXT("Invalid Coordinate [{}] for Grid Dimensions [{}]"), InCoordinate, InGridDimensions)
    { return INDEX_NONE; }

    const auto Index = InCoordinate.Y * InGridDimensions.X + InCoordinate.X;
    return Index;
}

auto
    UCk_Utils_Grid2D_UE::
    Get_IndexAsCoordinate(
        int32 InIndex,
        const FIntPoint& InGridDimensions)
    -> FIntPoint
{
    const auto TotalCells = Get_NumberOfCells(InGridDimensions);
    CK_ENSURE_IF_NOT(InIndex >= 0 && InIndex < TotalCells,
        TEXT("Invalid Index [{}] for Grid Dimensions [{}] (Total Cells: {})"), InIndex, InGridDimensions, TotalCells)
    { return {}; }

    const auto Coordinate = FIntPoint(
        InIndex % InGridDimensions.X,
        InIndex / InGridDimensions.X);
    return Coordinate;
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
        const FIntPoint& InCoordinate,
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
        const FIntPoint& InCoordinate,
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
        const FIntPoint& InCoordinate,
        FVector2D InCellSize)
    -> FVector2D
{
    const auto CellScale = FTransform{FRotator::ZeroRotator, FVector::ZeroVector, FVector(InCellSize.X, InCellSize.Y, 1.0f)};
    const auto ScaledCoordinate = CellScale.TransformPosition(FVector(InCoordinate.X, InCoordinate.Y, 0.0f));
    const auto WorldLocation = InTransform.TransformPosition(ScaledCoordinate);

    return FVector2D(WorldLocation.X, WorldLocation.Y);
}

auto
    UCk_Utils_Grid2D_UE::
    TransformCoordinate_AsLocation_Anchored(
        const FTransform& InTransform,
        const FIntPoint& InCoordinate,
        FVector2D InCellSize,
        const FIntPoint& InGridDimensions,
        ECk_GridAnchor InAnchor)
    -> FVector2D
{
    CK_ENSURE_IF_NOT(Get_IsValidCoordinate(InGridDimensions, InCoordinate),
        TEXT("Invalid Coordinate [{}] for Grid Dimensions [{}]"), InCoordinate, InGridDimensions)
    { return {}; }

    switch (InAnchor)
    {
        case ECk_GridAnchor::Default:
        {
            return TransformCoordinate_AsLocation(InTransform, InCoordinate, InCellSize);
        }
        case ECk_GridAnchor::Center:
        {
            const auto GridTranslationOffset = FVector(
                ((InCellSize.X * (InGridDimensions.X - 1)) / 2.0f),
                ((InCellSize.Y * (InGridDimensions.Y - 1)) / 2.0f),
                0.0f);

            const auto GridOffsetTransform = FTransform{FRotator::ZeroRotator, GridTranslationOffset, FVector::OneVector};
            const auto InvGridOffsetTransform = GridOffsetTransform.Inverse();
            const auto OffsetTransformToUse = InvGridOffsetTransform * InTransform;

            return TransformCoordinate_AsLocation(OffsetTransformToUse, InCoordinate, InCellSize);
        }
        default:
        {
            CK_INVALID_ENUM(InAnchor);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Grid3D_UE::
    Get_IsValidCoordinate(
        const FIntVector& InGridDimensions,
        const FIntVector& InCoordinate)
    -> bool
{
    const auto IsValid = InCoordinate.X >= 0 && InCoordinate.X < InGridDimensions.X &&
                         InCoordinate.Y >= 0 && InCoordinate.Y < InGridDimensions.Y &&
                         InCoordinate.Z >= 0 && InCoordinate.Z < InGridDimensions.Z;
    return IsValid;
}

auto
    UCk_Utils_Grid3D_UE::
    Get_NumberOfCells(
        const FIntVector& InGridDimensions)
    -> int32
{
    return InGridDimensions.X * InGridDimensions.Y * InGridDimensions.Z;
}

auto
    UCk_Utils_Grid3D_UE::
    Get_CoordinateAsIndex(
        const FIntVector& InCoordinate,
        const FIntVector& InGridDimensions)
    -> int32
{
    CK_ENSURE_IF_NOT(Get_IsValidCoordinate(InGridDimensions, InCoordinate),
        TEXT("Invalid Coordinate [{}] for Grid Dimensions [{}]"), InCoordinate, InGridDimensions)
    { return INDEX_NONE; }

    const auto Index = InCoordinate.Z * (InGridDimensions.X * InGridDimensions.Y) +
                       InCoordinate.Y * InGridDimensions.X +
                       InCoordinate.X;
    return Index;
}

auto
    UCk_Utils_Grid3D_UE::
    Get_IndexAsCoordinate(
        int32 InIndex,
        const FIntVector& InGridDimensions)
    -> FIntVector
{
    const auto TotalCells = Get_NumberOfCells(InGridDimensions);
    CK_ENSURE_IF_NOT(InIndex >= 0 && InIndex < TotalCells,
        TEXT("Invalid Index [{}] for Grid Dimensions [{}] (Total Cells: {})"), InIndex, InGridDimensions, TotalCells)
    { return {}; }

    const auto SliceSize = InGridDimensions.X * InGridDimensions.Y;
    const auto Z = InIndex / SliceSize;
    const auto Remainder = InIndex % SliceSize;
    const auto Y = Remainder / InGridDimensions.X;
    const auto X = Remainder % InGridDimensions.X;

    const auto Coordinate = FIntVector(X, Y, Z);
    return Coordinate;
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
        const FIntVector& InCoordinate,
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
        const FIntVector& InCoordinate,
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
        const FIntVector& InCoordinate,
        FVector InCellSize)
    -> FVector
{
    const auto CellScale = FTransform{FRotator::ZeroRotator, FVector::ZeroVector, InCellSize};
    const auto ScaledCoordinate = CellScale.TransformPosition(FVector(InCoordinate.X, InCoordinate.Y, InCoordinate.Z));
    const auto WorldLocation = InTransform.TransformPosition(ScaledCoordinate);

    return WorldLocation;
}

auto
    UCk_Utils_Grid3D_UE::
    TransformCoordinate_AsLocation_Anchored(
        const FTransform& InTransform,
        const FIntVector& InCoordinate,
        FVector InCellSize,
        const FIntVector& InGridDimensions,
        ECk_GridAnchor InAnchor)
    -> FVector
{
    CK_ENSURE_IF_NOT(Get_IsValidCoordinate(InGridDimensions, InCoordinate),
        TEXT("Invalid Coordinate [{}] for Grid Dimensions [{}]"), InCoordinate, InGridDimensions)
    { return {}; }

    switch (InAnchor)
    {
        case ECk_GridAnchor::Default:
        {
            return TransformCoordinate_AsLocation(InTransform, InCoordinate, InCellSize);
        }
        case ECk_GridAnchor::Center:
        {
            const auto GridTranslationOffset = FVector(
                ((InCellSize.X * (InGridDimensions.X - 1)) / 2.0f),
                ((InCellSize.Y * (InGridDimensions.Y - 1)) / 2.0f),
                ((InCellSize.Z * (InGridDimensions.Z - 1)) / 2.0f));

            const auto GridOffsetTransform = FTransform{FRotator::ZeroRotator, GridTranslationOffset, FVector::OneVector};
            const auto InvGridOffsetTransform = GridOffsetTransform.Inverse();
            const auto OffsetTransformToUse = InvGridOffsetTransform * InTransform;

            return TransformCoordinate_AsLocation(OffsetTransformToUse, InCoordinate, InCellSize);
        }
        default:
        {
            CK_INVALID_ENUM(InAnchor);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
