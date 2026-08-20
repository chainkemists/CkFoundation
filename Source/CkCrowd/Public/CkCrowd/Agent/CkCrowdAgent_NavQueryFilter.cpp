#include "CkCrowdAgent_NavQueryFilter.h"

#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_NavQueryFilter_AvoidStandingCrowds::UCk_NavQueryFilter_AvoidStandingCrowds()
{
    auto& Area = Areas.AddDefaulted_GetRef();
    Area.AreaClass = UCk_NavArea_CrowdAgent::StaticClass();
    Area.bIsExcluded = true;
}

// --------------------------------------------------------------------------------------------------------------------
