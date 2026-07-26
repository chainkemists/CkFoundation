#pragma once

#include "CkPathNetwork/Network/CkPathNetwork_BuiltNetwork.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Ribbons -> built network. Pure math, runtime-callable, no ECS/world dependency. Junctions are
// never authored: an endpoint within _NodeSnapRadius of another ribbon's INTERIOR splits it, and all
// post-split endpoints within _NodeSnapRadius fuse into one node. X-crossings are NOT auto-junctioned.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    CKPATHNETWORK_API auto
    Build_NetworkFromRibbons(
        const TArray<FCk_PathNetwork_Ribbon>& InRibbons,
        const FCk_PathNetwork_BuildParams& InParams) -> FBuiltNetwork;
}

// --------------------------------------------------------------------------------------------------------------------
