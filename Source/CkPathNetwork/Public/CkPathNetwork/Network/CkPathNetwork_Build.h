#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Ribbons -> built network. Pure math, runtime-callable, no ECS/world dependency.
//
// Junction resolution is entirely automatic:
//  - T-junctions: a ribbon endpoint within _NodeSnapRadius of another ribbon's INTERIOR splits
//    that ribbon at the projection.
//  - Endpoint fusion: all (post-split) endpoints within _NodeSnapRadius of each other cluster
//    into one node; edge end points are snapped to the node centroid.
//
// Mid-polyline X-crossings between two ribbons with no endpoint nearby are NOT auto-junctioned —
// detector-generated ribbons never produce them (the skeleton splits at junction pixels) and
// hand-drawn ribbons should end at intersections (the editor tool's validation warns).
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    CKPATHNETWORK_API auto
    Build_NetworkFromRibbons(
        const TArray<FCk_PathNetwork_Ribbon>& InRibbons,
        const FCk_PathNetwork_BuildParams& InParams) -> FBuiltNetwork;
}

// --------------------------------------------------------------------------------------------------------------------
