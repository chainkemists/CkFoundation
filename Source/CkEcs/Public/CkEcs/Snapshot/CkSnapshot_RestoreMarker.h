#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Stamped by the load on every entity it mapped, beside the hydration quarantine, and NEVER removed: it
    // records that this entity was part of a load during this session, not that anything is currently pending.
    // A consumer that wants "my restored state is final" wants the one-shot Promise_OnHydrated instead — this
    // is a permanent fact, so every reader has to carry its own dedup, which is the trap it is named after.
    CK_DEFINE_ECS_TAG(FTag_Hydration_WasHydratedThisLoad);

    // Stamp at creation on any entity that is DERIVED state its owner's construction/redrive recreates on
    // load (canonical: the SM graph). Unstamped, the capture persists it via its SpawnRecipe and respawns
    // it as a top-level duplicate that re-runs its lifecycle outside the owning feature's context.
    CK_DEFINE_ECS_TAG(FTag_Snapshot_SaveTransient);

    // Reconstruct-only marker: this entity is intentionally local/derived state whose owning feature creates from
    // authored defaults after a load boundary. Unlike FTag_Snapshot_SaveTransient, its persistence handlers may
    // legitimately be able to Produce a payload; capture deliberately omits both the entity row and that payload,
    // and reconcile leaves the rebuilt child alone. Use this only where resetting is the declared load policy (for
    // example a per-viewer camera's non-replicated tuner attributes), never to silence an accidental data loss.
    CK_DEFINE_ECS_TAG(FTag_Snapshot_ReconstructOnly);

    // Stamped by the load on an entity whose recipe named an archetype that NOTHING could resolve - neither the
    // path itself nor any registered runtime-archetype provider. Construction fell back to the class default, so
    // the entity is structurally an entity and semantically nothing: an item with no traits, a husk that still
    // occupies its inventory slot and its grid cell. The load reaps every entity carrying this before it hands the
    // world back, which is what keeps an unresolvable recipe a NAMED loss rather than a permanently dead slot.
    //
    // The husk is built at all - rather than the row being dropped - because the inventory hydration handlers are
    // all-or-nothing: one invalid handle makes the whole container return NotReady until it times out, so dropping
    // one broken item would cost the player the entire container it lived in.
    CK_DEFINE_ECS_TAG(FTag_Snapshot_UnresolvedArchetype);
}

// --------------------------------------------------------------------------------------------------------------------
