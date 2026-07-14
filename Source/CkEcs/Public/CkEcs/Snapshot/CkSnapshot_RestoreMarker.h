#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Legacy restore marker, deleted with Model A. Nothing in CkFoundation stamps or reads it any longer — the load path
    // (UCk_Snapshot_Subsystem::Request_Load) never stamps it, so Has<FTag_Snapshot_JustRestored> is always
    // false at runtime. RETAINED so the BB superproject still compiles against this header + symbol
    // (Bb_SnapshotRestore.cpp / Bb_CombatReceiverRestore.cpp, and the BB AS restore-rebind fleet that already
    // early-outs on the always-false result); deleting this file is a BB-repo task, done together with that
    // BB-side cleanup. Transient ⇒ no snapshot-registrar side effect, so retention is inert.
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
