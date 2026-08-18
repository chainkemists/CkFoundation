#pragma once

#include "CkEcs/Tag/CkTag.h"

#include <HAL/Platform.h>

// --------------------------------------------------------------------------------------------------------------------
// Its own small header so foundational code can pick these up without dragging the hydration payload types
// (FInstancedStruct, a UCLASS and its generated header) into every translation unit that includes the registry:
// FCk_Registry::Clear consults the context, and the processor view helper appends the tag as an exclusion, so both
// CkRegistry.h and CkProcessor.h need them. Same reason CkTag_EditorOnly.h exists.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // On a mapped entity while the load still owns its Durable state. Every non-exempt processor excludes it, and a
    // registry-wide Clear leaves it alone. Stamped and lifted for the WHOLE mapped set at once, never per entity:
    // the exclusion is a view filter, not a memory barrier, so a released entity reading a still-quarantined
    // sibling's fragment directly is exactly what a per-entity lift would allow.
    CK_DEFINE_ECS_TAG(FTag_Hydration_Quarantine);

    // Context-key type for the quarantine's population count. Lives in entt::registry::ctx() so every FCk_Registry
    // view bound to the same slot sees it, however that view was constructed, and so the two hot paths (Clear,
    // view construction) can ask "is a quarantine active at all" without touching a pool.
    struct FCtx_HydrationQuarantine
    {
        int32 _Count = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
