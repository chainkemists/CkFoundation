#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <Math/TransformCalculus2D.h>

#include "Ck2dGridSystem_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridSystem : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridSystem);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_DebugDraw_CellVisualization : uint8
{
    None,
    AABB,
    OBB
};

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

UENUM(BlueprintType)
enum class ECk_2dGridSystem_PivotAnchor : uint8
{
    Center,
    BottomLeft,
    BottomCenter,
    BottomRight,
    MiddleLeft,
    MiddleRight,
    TopLeft,
    TopCenter,
    TopRight
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_PivotAnchor);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridSystem_DebugDraw_Options
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridSystem_DebugDraw_Options);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_2dGridSystem_DebugDraw_CellVisualization _CellVisualization = ECk_2dGridSystem_DebugDraw_CellVisualization::OBB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowCoordinates = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowPivot = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowCellSizeInfo = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _EnabledCellColor = FLinearColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _DisabledCellColor = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _PivotColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _CellThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PivotSize = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Duration = 0.0f;

public:
    CK_PROPERTY(_CellVisualization);
    CK_PROPERTY(_ShowCoordinates);
    CK_PROPERTY(_ShowPivot);
    CK_PROPERTY(_ShowCellSizeInfo);
    CK_PROPERTY(_EnabledCellColor);
    CK_PROPERTY(_DisabledCellColor);
    CK_PROPERTY(_PivotColor);
    CK_PROPERTY(_TextColor);
    CK_PROPERTY(_CellThickness);
    CK_PROPERTY(_PivotSize);
    CK_PROPERTY(_Duration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridSystem_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridSystem_Spec);

private:
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector2D _CellSize = FVector2D(100.0f, 100.0f);

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _DefaultCellState = ECk_EnableDisable::Enable;

    // Coordinates that should be the opposite of the default state
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _ExceptionCoordinates;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _Pivot;

    // Tags applied to EVERY cell of this grid by default. Unioned with each cell's own
    // _Tags when the placement layer evaluates an object's required/forbidden cell-tag gating.
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "2dGridCell"))
    FGameplayTagContainer _DefaultCellTags;

public:
    CK_PROPERTY(_Dimensions);
    CK_PROPERTY(_CellSize);
    CK_PROPERTY(_DefaultCellState);
    CK_PROPERTY(_ExceptionCoordinates);
    CK_PROPERTY(_Pivot);
    CK_PROPERTY(_DefaultCellTags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_2dGridSystem_Spec, _Dimensions, _CellSize);

public:
    auto
    Get_ResolvedActiveCoordinates() const -> TArray<FIntPoint>;

    auto
    Get_IsCoordinateActive(
        const FIntPoint& InCoordinate) const -> bool;
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
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_GridCellIntersection> _IntersectingCells;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _TotalIntersections = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _GridAFullyContainedInGridB = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _GridBFullyContainedInGridA = false;

    // Fraction of GridA's filter-matched cells that intersect GridB (and vice versa).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GridAOverlapPercent = 0.0f;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _HasValidSnapPosition = false;

public:
    CK_PROPERTY(_IntersectingCells);
    CK_PROPERTY(_TotalIntersections);
    CK_PROPERTY(_GridAFullyContainedInGridB);
    CK_PROPERTY(_GridBFullyContainedInGridA);
    CK_PROPERTY(_GridAOverlapPercent);
    CK_PROPERTY(_GridBOverlapPercent);
    CK_PROPERTY(_TotalIntersectionBounds);
    CK_PROPERTY(_SnapPosition);
    CK_PROPERTY(_HasValidSnapPosition);
};

// --------------------------------------------------------------------------------------------------------------------