#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment_Data.h"

#include <NavAreas/NavArea.h>

#include "CkCrowdAvoidanceVolume_NavArea.generated.h"

// The costs are ascending because Recast's default area-modifier sorting applies higher-cost
// modifiers last. That makes HardExclude win wherever authored volumes overlap.
UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAvoidanceVolume : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAvoidanceVolume();
};

UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAvoidanceVolume_CostOnly : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAvoidanceVolume_CostOnly();
};

UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAvoidanceVolume_HardExclude : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAvoidanceVolume_HardExclude();
};

// --------------------------------------------------------------------------------------------------------------------
