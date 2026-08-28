#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NavAreas/NavArea.h>

#include "CkCrowdAvoidanceVolume_NavArea.generated.h"

// A finite cost keeps topology connected while Crowd's query overlay excludes this area.
UCLASS()
class CKCROWD_API UCk_NavArea_CrowdAvoidanceVolume : public UNavArea
{
    GENERATED_BODY()

public:
    UCk_NavArea_CrowdAvoidanceVolume();
};

// --------------------------------------------------------------------------------------------------------------------
