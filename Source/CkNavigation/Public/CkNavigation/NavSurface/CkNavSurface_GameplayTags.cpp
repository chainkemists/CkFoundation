#include "CkNavigation/NavSurface/CkNavSurface_GameplayTags.h"

#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

#include <NavAreas/NavArea_Null.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Restricted, TEXT("Nav.Area.Restricted"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Nav_Area_Impassable, TEXT("Nav.Area.Impassable"));

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_gameplay_tags
{
    const auto Registration = ck::nav_surface_recast::FRegistrar{[]
    {
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Impassable, UNavArea_Null::StaticClass());
    }};

    const auto PolicyRegistration = ck::nav_surface::FRegistrar{[]
    {
        // The multiplier is inert next to Walkability: nothing crosses this area at any price.
        ck::nav_surface::Register_AreaPolicy(TAG_Nav_Area_Impassable,
            FCk_NavSurface_AreaPolicy{ECk_NavSurface_AreaPolicyKind::Walkability, 1.0f});
    }};
}

// --------------------------------------------------------------------------------------------------------------------
