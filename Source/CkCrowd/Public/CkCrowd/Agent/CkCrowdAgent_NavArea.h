#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NavAreas/NavArea.h>

#include "CkCrowdAgent_NavArea.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// COST semantics, deliberately not exclusion and not a hole — a fully-plugged corridor must stay
// traversable and the mesh under the agent must stay walkable. See CkCrowd/CLAUDE.md.
UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAgent : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAgent();
};

// --------------------------------------------------------------------------------------------------------------------
