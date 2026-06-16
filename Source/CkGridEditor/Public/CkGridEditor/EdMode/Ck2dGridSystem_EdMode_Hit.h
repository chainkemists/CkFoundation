#pragma once

#include "CoreMinimal.h"

#include "Math/IntPoint.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"

#include "Misc/Optional.h"

// --------------------------------------------------------------------------------------------------------------------

// Pure ray->cell math for the Grid Paint editor mode. Deliberately free of any editor/world/UObject
// dependency so it is unit-testable in isolation (see Test_GridEdMode_Hit.cpp).
//
// Coordinate convention (matches the runtime grid, see UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation
// in CkGrid_Utils.cpp): grid-local space has cell (0,0)'s MIN corner at the local origin; cell (x,y)
// spans local [x*CellSize.X, (x+1)*CellSize.X) on X and [y*CellSize.Y, (y+1)*CellSize.Y) on Y, on the
// z=0 plane. InGridTransform maps grid-local to world. A cell coordinate is valid when it lies in
// [0, Dimensions) on both axes.
namespace ck::grid_editor
{
    // Intersects the world-space ray with the grid's local z=0 plane and returns the cell coordinate
    // the hit lands in, or unset if the ray is parallel to the plane, hits behind the origin, or lands
    // outside [0, Dimensions). InRayDirection need not be normalized.
    CKGRIDEDITOR_API auto
    Resolve_CellFromRay(
        const FTransform& InGridTransform,
        FVector2D         InCellSize,
        FIntPoint         InDimensions,
        const FVector&    InRayOrigin,
        const FVector&    InRayDirection) -> TOptional<FIntPoint>;

    // Enumerates every in-bounds cell of the inclusive rectangle spanning InCornerA..InCornerB (corners in
    // any order). Coordinates are clamped to [0, InDimensions); a rectangle fully outside the grid yields an
    // empty array. Used by the box-select rect fill for the Shape and Tags tools.
    CKGRIDEDITOR_API auto
    Compute_RectCells(
        const FIntPoint& InCornerA,
        const FIntPoint& InCornerB,
        const FIntPoint& InDimensions) -> TArray<FIntPoint>;
}

// --------------------------------------------------------------------------------------------------------------------
