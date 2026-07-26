#include "CkCrowdAgent_NavArea.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_NavArea_CrowdAgent::UCk_NavArea_CrowdAgent()
{
    constexpr auto StandingAgentTollCost = 64.0f;
    DefaultCost = StandingAgentTollCost;
    DrawColor = FColor::Orange;
}

// --------------------------------------------------------------------------------------------------------------------
