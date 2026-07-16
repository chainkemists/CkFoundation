#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NavAreas/NavArea.h>

#include "CkCrowdAgent_NavArea.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Painted under an agent that has been stationary for a while (see
// FProcessor_CrowdAgent_StationaryMarkup). COST semantics, deliberately not exclusion and not a
// hole: pathfinding detours around standing agents when a detour exists, but a fully-plugged
// corridor still paths through rather than failing — and the mesh under the agent stays walkable,
// so the agent's own navmesh clamp, path starts, and slot projections are unaffected.
UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAgent : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAgent();
};

// --------------------------------------------------------------------------------------------------------------------
