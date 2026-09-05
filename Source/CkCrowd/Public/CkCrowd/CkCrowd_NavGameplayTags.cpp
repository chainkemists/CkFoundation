#include "CkCrowd/CkCrowd_NavGameplayTags.h"

#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_NavArea.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include <NavAreas/NavArea.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_Agent, TEXT("Nav.Area.Crowd.Agent"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume, TEXT("Nav.Area.Crowd.AvoidanceVolume"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly, TEXT("Nav.Area.Crowd.AvoidanceVolume.CostOnly"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude, TEXT("Nav.Area.Crowd.AvoidanceVolume.HardExclude"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Filter_Crowd_AvoidStandingCrowds, TEXT("Nav.Filter.Crowd.AvoidStandingCrowds"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_nav_gameplay_tags
{
    // The area class's authored DefaultCost IS the neutral policy's multiplier — a provider without
    // UNavArea gets the same number Recast reads off the CDO, from the one place it is written.
    auto TryGet_CostPolicy(
        TSubclassOf<UNavArea> InAreaClass) -> TOptional<FCk_NavSurface_AreaPolicy>
    {
        const auto* AreaDefaults = InAreaClass.GetDefaultObject();

        const auto AreaDefaultsAreValid = ck::IsValid(AreaDefaults);
        CK_ENSURE_IF_NOT(AreaDefaultsAreValid,
            TEXT("Crowd nav area class [{}] has no class default object to read its traversal cost from"),
            GetNameSafe(InAreaClass.Get()))
        { return {}; }

        return FCk_NavSurface_AreaPolicy{
            ECk_NavSurface_AreaPolicyKind::Cost, AreaDefaults->DefaultCost};
    }

    auto DoRegister_CostPolicy(
        const FGameplayTag&   InAreaTag,
        TSubclassOf<UNavArea> InAreaClass) -> void
    {
        const auto Policy = TryGet_CostPolicy(InAreaClass);

        if (NOT Policy.IsSet())
        { return; }

        ck::nav_surface::Register_AreaPolicy(InAreaTag, *Policy);
    }

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

    const auto PolicyRegistration = ck::nav_surface::FRegistrar{[]
    {
        DoRegister_CostPolicy(
            TAG_Nav_Area_Crowd_Agent, UCk_NavArea_CrowdAgent::StaticClass());
        DoRegister_CostPolicy(
            TAG_Nav_Area_Crowd_AvoidanceVolume, UCk_NavArea_CrowdAvoidanceVolume::StaticClass());
        DoRegister_CostPolicy(
            TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly, UCk_NavArea_CrowdAvoidanceVolume_CostOnly::StaticClass());

        // A hard exclusion is a PRICE on both providers, not a walkability rejection: Recast carries it
        // as the area class's prohibitive DefaultCost, and the neutral policy reads that same number off
        // the class, so a hard-exclude volume is as expensive on GroundNav as it is on Recast. The one
        // area that removes walkability is Nav.Area.Impassable, and this is not it.
        DoRegister_CostPolicy(
            TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude, UCk_NavArea_CrowdAvoidanceVolume_HardExclude::StaticClass());
    }};
}

// --------------------------------------------------------------------------------------------------------------------
