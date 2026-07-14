#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Mask -> ribbons. Pure math, runtime-callable, no ECS/world dependency.
//
// Pipeline: chamfer distance transform (half-width estimation) -> Zhang-Suen thinning (skeleton)
// -> chain tracing between junction/end pixels -> Douglas-Peucker simplification -> ribbons.
// Output ribbons are marked ECk_PathNetwork_RibbonSource::Generated and get fresh ids.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    CKPATHNETWORK_API auto
    Vectorize_MaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams) -> TArray<FCk_PathNetwork_Ribbon>;
}

// --------------------------------------------------------------------------------------------------------------------
