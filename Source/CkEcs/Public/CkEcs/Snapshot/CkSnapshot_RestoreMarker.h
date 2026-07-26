#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stamped by the load on every restored entity before the load gate opens, so game-side rebind
    // processors can re-resolve the handles their persisted fragments carry. It survives on the entity
    // for its whole lifetime, so consumers must pair it with their own once-per-feature dedup.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_JustRestored);

    // Stamp at creation on any entity that is DERIVED state its owner's construction/redrive recreates on
    // load (canonical: the SM graph). Unstamped, the capture persists it via its SpawnRecipe and respawns
    // it as a top-level duplicate that re-runs its lifecycle outside the owning feature's context.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_SaveTransient);
}

// --------------------------------------------------------------------------------------------------------------------
