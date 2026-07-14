#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Restore marker: stamped by the v3 load (UCk_Snapshot_Subsystem::DoHydrate_Enqueue) on every restored
    // (saved-id-mapped) entity, before the load gate opens. Game-side rebind processors key off it to
    // re-resolve handles their persisted fragments carry (e.g. BB's Bb_SnapshotRestore rebind fleet).
    // The Model-A purge briefly deleted the stamp site (2026-07-13), silently no-op'ing every consumer —
    // re-stamped under v3 on 2026-07-14. Transient ⇒ never captured; survives on the entity for its
    // lifetime, so consumers pair it with their own once-per-feature dedup (Bb: Get_RebindDone).
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_JustRestored);

    // Save-transient marker: this entity is DERIVED state whose owner's construction/redrive recreates
    // it on load — the v3 capture must never persist it as a respawnable row. Stamp it at creation on
    // entities a feature rebuilds itself (canonical: the SM graph — states/tasks/conditions/transitions/
    // sub-SMs, recreated by the SM hydration redrive). Without the stamp, such entities are captured via
    // their SpawnRecipe (RuntimeSpawned) and respawned as top-level duplicates that re-run their lifecycle
    // outside the owning feature's context (the zombie-SmTask-with-destroyed-SM incident, 2026-07-14).
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_SaveTransient);
}

// --------------------------------------------------------------------------------------------------------------------
