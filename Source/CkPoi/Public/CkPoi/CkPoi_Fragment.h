#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Identity tag for the POI meta-feature. Composed by UCk_Utils_Poi_UE::Add alongside the CkEntityTag category
    // (+ optional CkLabel). Projectors gather POIs via View<FTag_Poi>. Mirrors CkProjectile's FTag_Projectile.
    CK_DEFINE_ECS_TAG(FTag_Poi);
}

// --------------------------------------------------------------------------------------------------------------------
