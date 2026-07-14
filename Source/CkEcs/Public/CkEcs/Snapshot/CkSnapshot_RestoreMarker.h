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
}

// --------------------------------------------------------------------------------------------------------------------
