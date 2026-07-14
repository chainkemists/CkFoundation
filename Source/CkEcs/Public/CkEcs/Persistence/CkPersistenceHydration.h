#pragma once

// Load-path hydration bookkeeping — split out of Net/ReplicatedFragmentContainer/ (saveload-v3-ergonomics Phase 5).
// The save/load path fills FFragment_PendingHydration; FProcessor_Hydration_Dispatch drains it through the
// registered HydrationApply. Transport-neutral (no Net/ dependency).

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include <InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Set on an entity that has local (save-load) hydration payloads queued for Apply — the load-path
    // counterpart to net-received container entries. Drained by FProcessor_Hydration_Dispatch. The
    // load path fills the queue below.
    CK_DEFINE_ECS_TAG(FTag_Hydration_PendingApply);

    // Local hydration queue: payloads to apply on this entity via the SAME handler HydrationApply the net path's
    // sibling uses, but sourced from a save load rather than the wire. Transient bookkeeping — not persisted.
    struct FFragment_PendingHydration
    {
        CK_GENERATED_BODY(FFragment_PendingHydration);

    public:
        TArray<FInstancedStruct> _Entries;
        float _PendingForSeconds = 0.0f;
    };
}

// --------------------------------------------------------------------------------------------------------------------
