#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include <Math/TransformCalculus2D.h>

#include "Ck2dGridSystem_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridSystem : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_CellFilter : uint8
{
    OnlyActiveCells,
    OnlyDisabledCells,
    NoFilter UMETA(DisplayName = "No Filter (All Cells)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_CellFilter);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_CoordinateType : uint8
{
    Local,     // Original coordinates, unaffected by grid rotation
    Rotated    // Coordinates after applying grid rotation
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_CoordinateType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_Fragment_2dGridSystem_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_2dGridSystem_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _CellSize = FVector2D(100.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _ActiveCoordinates;

public:
    CK_PROPERTY(_Dimensions);
    CK_PROPERTY(_CellSize);
    CK_PROPERTY(_ActiveCoordinates);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_2dGridSystem_ParamsData, _Dimensions, _CellSize, _ActiveCoordinates);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_GridCellIntersection
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GridCellIntersection);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_2dGridCell _CellA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_2dGridCell _CellB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _CoordinateA = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _CoordinateB = FIntPoint::ZeroValue;

    // Intersection overlap percentage (0.0 to 1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _OverlapPercent = 0.0f;

    // World space bounds of the intersection area
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FBox2D _IntersectionBounds = FBox2D(ForceInit);

public:
    CK_PROPERTY(_CellA);
    CK_PROPERTY(_CellB);
    CK_PROPERTY(_CoordinateA);
    CK_PROPERTY(_CoordinateB);
    CK_PROPERTY(_OverlapPercent);
    CK_PROPERTY(_IntersectionBounds);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_GridIntersectionResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GridIntersectionResult);

private:
    // All individual cell intersections
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_GridCellIntersection> _IntersectingCells;

    // Summary statistics
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _TotalIntersections = 0;

    // Indicates if GridA is completely contained within GridB's active area
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _GridAFullyContainedInGridB = false;

    // Indicates if GridB is completely contained within GridA's active area
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _GridBFullyContainedInGridA = false;

    // Indicates if the grids have substantial overlap (configurable threshold)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _HasSubstantialOverlap = false;

    // Percentage of GridA's active cells that intersect with GridB
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GridAOverlapPercent = 0.0f;

    // Percentage of GridB's active cells that intersect with GridA
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GridBOverlapPercent = 0.0f;

    // Combined bounding box of all intersections in world space
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FBox2D _TotalIntersectionBounds = FBox2D(ForceInit);

    // World position where GridB should be moved to perfectly align with GridA
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _SnapPosition = FVector2D::ZeroVector;

    // Whether a valid snap position was calculated
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _HasValidSnapPosition = false;

public:
    CK_PROPERTY(_IntersectingCells);
    CK_PROPERTY(_TotalIntersections);
    CK_PROPERTY(_GridAFullyContainedInGridB);
    CK_PROPERTY(_GridBFullyContainedInGridA);
    CK_PROPERTY(_HasSubstantialOverlap);
    CK_PROPERTY(_GridAOverlapPercent);
    CK_PROPERTY(_GridBOverlapPercent);
    CK_PROPERTY(_TotalIntersectionBounds);
    CK_PROPERTY(_SnapPosition);
    CK_PROPERTY(_HasValidSnapPosition);
};

// --------------------------------------------------------------------------------------------------------------------