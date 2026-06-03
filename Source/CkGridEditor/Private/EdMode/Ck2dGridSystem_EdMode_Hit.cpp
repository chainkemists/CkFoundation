#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode_Hit.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::grid_editor
{
    auto
        Resolve_CellFromRay(
            const FTransform& InGridTransform,
            FVector2D         InCellSize,
            FIntPoint         InDimensions,
            const FVector&    InRayOrigin,
            const FVector&    InRayDirection) -> TOptional<FIntPoint>
    {
        if (InCellSize.X <= 0.0 || InCellSize.Y <= 0.0)
        { return {}; }

        if (InDimensions.X <= 0 || InDimensions.Y <= 0)
        { return {}; }

        // Work in grid-local space: the grid plane is z=0 there, so the intersection is a trivial
        // ratio along z. This also folds in any grid rotation/scale via the inverse transform.
        const auto LocalOrigin    = InGridTransform.InverseTransformPosition(InRayOrigin);
        const auto LocalDirection = InGridTransform.InverseTransformVector(InRayDirection);

        if (FMath::IsNearlyZero(LocalDirection.Z))
        { return {}; }

        const auto DistanceAlongRay = -LocalOrigin.Z / LocalDirection.Z;
        if (DistanceAlongRay < 0.0)
        { return {}; }

        const auto LocalHit = LocalOrigin + (LocalDirection * DistanceAlongRay);

        // Corner-origin convention: cell index is the floored local position divided by cell size.
        const auto CellX = FMath::FloorToInt(LocalHit.X / InCellSize.X);
        const auto CellY = FMath::FloorToInt(LocalHit.Y / InCellSize.Y);

        if (CellX < 0 || CellX >= InDimensions.X ||
            CellY < 0 || CellY >= InDimensions.Y)
        { return {}; }

        return FIntPoint(CellX, CellY);
    }
}

// --------------------------------------------------------------------------------------------------------------------
