#pragma once

#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // An entity is EITHER Chaos-simulated OR Jolt-simulated, never both (the two engines maintain
    // independent, non-interacting worlds). COUNTED because same-world features may stack; only the
    // cross-world claim is a conflict. See CkEcsExt/CLAUDE.md § "Physics ownership".
    CK_DEFINE_ECS_TAG_COUNTED(FTag_PhysicsOwnership_Chaos);
    CK_DEFINE_ECS_TAG_COUNTED(FTag_PhysicsOwnership_Jolt);
}

// --------------------------------------------------------------------------------------------------------------------
