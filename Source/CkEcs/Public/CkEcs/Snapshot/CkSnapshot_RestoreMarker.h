#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // RETIRED (Phase 5 cluster 3) — legacy Model-A restore re-drive marker. Its only stamper was Model-A
    // Run_Restore (deleted this phase) and its only consumers were the per-feature restore re-drive processors
    // (also deleted). The v3 load path (UCk_Snapshot_Subsystem::Request_Load) never stamps it, so
    // Has<FTag_Snapshot_JustRestored> is always false at runtime and NOTHING in CkFoundation stamps or reads it.
    // KEPT ONLY so the BB superproject still compiles against this header + symbol (Bb_SnapshotRestore.cpp /
    // Bb_CombatReceiverRestore.cpp, and the BB AS restore-rebind fleet that already early-outs on the always-false
    // result). Same cross-repo retention as FTag_ActorJustRebound; deleting this file is a BB-repo task, done
    // together with that BB-side cleanup. Transient ⇒ no snapshot-registrar side effect, so retention is inert.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_JustRestored);
}

// --------------------------------------------------------------------------------------------------------------------
