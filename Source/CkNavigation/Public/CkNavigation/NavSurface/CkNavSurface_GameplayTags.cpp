#include "CkNavigation/NavSurface/CkNavSurface_GameplayTags.h"

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
}

// --------------------------------------------------------------------------------------------------------------------
