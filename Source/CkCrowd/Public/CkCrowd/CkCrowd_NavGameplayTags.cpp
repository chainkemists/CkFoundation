#include "CkCrowd/CkCrowd_NavGameplayTags.h"

#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"

#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_Agent, TEXT("Nav.Area.Crowd.Agent"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume, TEXT("Nav.Area.Crowd.AvoidanceVolume"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly, TEXT("Nav.Area.Crowd.AvoidanceVolume.CostOnly"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude, TEXT("Nav.Area.Crowd.AvoidanceVolume.HardExclude"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Filter_Crowd_AvoidStandingCrowds, TEXT("Nav.Filter.Crowd.AvoidStandingCrowds"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_nav_gameplay_tags
{
    const auto Registration = ck::nav_surface_recast::FRegistrar{[]
    {
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Crowd_Agent, UCk_NavArea_CrowdAgent::StaticClass());
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Crowd_AvoidanceVolume, UCk_NavArea_CrowdAvoidanceVolume::StaticClass());
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly, UCk_NavArea_CrowdAvoidanceVolume_CostOnly::StaticClass());
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude, UCk_NavArea_CrowdAvoidanceVolume_HardExclude::StaticClass());

        auto StrictDefinition = FCk_NavFilter_Definition{};
        StrictDefinition.Set_ExcludedAreaTags(FGameplayTagContainer{TAG_Nav_Area_Crowd_Agent.GetTag()});
        ck::nav_surface_recast::Register_FilterDefinition(
            TAG_Nav_Filter_Crowd_AvoidStandingCrowds, MoveTemp(StrictDefinition));
    }};
}

// --------------------------------------------------------------------------------------------------------------------
