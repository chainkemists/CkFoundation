#include "CkNavArea_Restricted.h"

#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/CkNavSurface_GameplayTags.h"
#include "CkNavigation/NavSurface/Recast/CkNavSurface_RecastAdapter.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_area_restricted
{
    const auto Registration = ck::nav_surface_recast::FRegistrar{[]
    {
        ck::nav_surface_recast::Register_AreaTag(
            TAG_Nav_Area_Restricted, UCk_NavArea_Restricted::StaticClass());
    }};

    const auto PolicyRegistration = ck::nav_surface::FRegistrar{[]
    {
        // The area class authors no DefaultCost, so crossing it costs what unmarked surface costs.
        // Which filters refuse it entirely is each filter's decision, not this area's meaning.
        ck::nav_surface::Register_AreaPolicy(TAG_Nav_Area_Restricted,
            FCk_NavSurface_AreaPolicy{ECk_NavSurface_AreaPolicyKind::Cost, 1.0f});
    }};
}

// --------------------------------------------------------------------------------------------------------------------

UCk_NavArea_Restricted::UCk_NavArea_Restricted()
{
    DrawColor = FColor::Red;
}

// --------------------------------------------------------------------------------------------------------------------
