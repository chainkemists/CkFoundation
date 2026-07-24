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

    // Reconstruct-only marker: this entity is intentionally local/derived state whose owning feature creates from
    // authored defaults after a load boundary. Unlike FTag_Snapshot_SaveTransient, its persistence handlers may
    // legitimately be able to Produce a payload; capture deliberately omits both the entity row and that payload,
    // and reconcile leaves the rebuilt child alone. Use this only where resetting is the declared load policy (for
    // example a per-viewer camera's non-replicated tuner attributes), never to silence an accidental data loss.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_ReconstructOnly);
}

// --------------------------------------------------------------------------------------------------------------------
