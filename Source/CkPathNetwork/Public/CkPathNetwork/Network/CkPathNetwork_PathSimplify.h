#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    // Greedily removes path points only where the caller confirms the direct segment is traversable.
    // Removed points must also remain close to the linear height along that segment's planar chord.
    // Invalid tolerances preserve the original path unchanged.
    CKPATHNETWORK_API auto
    Simplify_PathByTraversal(
        TConstArrayView<FVector> InPath,
        TFunctionRef<bool(const FVector&, const FVector&)> InIsShortcutTraversable,
        float InMaximumVerticalDeviationCm)
        -> TArray<FVector>;
}

// --------------------------------------------------------------------------------------------------------------------
