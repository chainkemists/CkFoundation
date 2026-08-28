#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"

UCk_NavArea_CrowdAvoidanceVolume::UCk_NavArea_CrowdAvoidanceVolume()
{
    DefaultCost = 4096.0f;
    DrawColor = FColor{255, 214, 64};
}

// --------------------------------------------------------------------------------------------------------------------

UCk_NavArea_CrowdAvoidanceVolume_CostOnly::UCk_NavArea_CrowdAvoidanceVolume_CostOnly()
{
    DefaultCost = 1024.0f;
    DrawColor = FColor{96, 180, 255};
}

// --------------------------------------------------------------------------------------------------------------------

UCk_NavArea_CrowdAvoidanceVolume_HardExclude::UCk_NavArea_CrowdAvoidanceVolume_HardExclude()
{
    DefaultCost = 16384.0f;
    DrawColor = FColor{255, 80, 80};
}

// --------------------------------------------------------------------------------------------------------------------

auto ck::crowd_avoidance_volume::Get_NavAreaClass(
    ECk_CrowdAvoidanceVolume_TraversalPolicy InTraversalPolicy) -> TSubclassOf<UNavArea>
{
    switch (InTraversalPolicy)
    {
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::AvoidIfPossible:
            return UCk_NavArea_CrowdAvoidanceVolume::StaticClass();
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::HardExclude:
            return UCk_NavArea_CrowdAvoidanceVolume_HardExclude::StaticClass();
        case ECk_CrowdAvoidanceVolume_TraversalPolicy::CostOnly:
            return UCk_NavArea_CrowdAvoidanceVolume_CostOnly::StaticClass();
        default:
            return {};
    }
}
