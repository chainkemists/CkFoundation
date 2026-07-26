#pragma once

#include "CoreMinimal.h"

#include "Math/IntPoint.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"

#include "Misc/Optional.h"

// --------------------------------------------------------------------------------------------------------------------

// Pure ray->cell math, deliberately free of any editor/world/UObject dependency so it is unit-testable
// in isolation. Coordinate convention (matches UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation): cell (x,y)
// spans local [x*CellSize.X, (x+1)*CellSize.X) x [y*CellSize.Y, (y+1)*CellSize.Y) on the z=0 plane.
namespace ck::grid_editor
{
    // Unset if the ray is parallel to the plane, hits behind the origin, or lands outside [0, Dimensions).
    // InRayDirection need not be normalized.
    CKGRIDEDITOR_API auto
    Resolve_CellFromRay(
        const FTransform& InGridTransform,
        FVector2D         InCellSize,
        FIntPoint         InDimensions,
        const FVector&    InRayOrigin,
        const FVector&    InRayDirection) -> TOptional<FIntPoint>;

    // Inclusive rectangle, corners in any order, clamped to [0, InDimensions); fully outside yields empty.
    CKGRIDEDITOR_API auto
    Compute_RectCells(
        const FIntPoint& InCornerA,
        const FIntPoint& InCornerB,
        const FIntPoint& InDimensions) -> TArray<FIntPoint>;
}

// --------------------------------------------------------------------------------------------------------------------
