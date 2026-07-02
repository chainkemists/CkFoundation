#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Transient marker stamped by CkSnapshot on EVERY entity restored by a load (Run_Restore), AFTER the lifecycle
    // strip. Replicated features key off it to RE-DRIVE replication of restored VALUES to clients: a per-feature
    // replication trigger tag (e.g. an attribute's FTag_MayRequireReplication) is transient and is consumed before a
    // save, so it is NOT part of the snapshot. Without re-driving, a client keeps the Construct-default value for a
    // restored entity (the server holds the restored value, but never re-pushes it). Each replicated feature adds a
    // small processor that, once the owner's replication driver is re-established (snapshot respawn pass), re-arms its
    // replication and clears this marker.
    //
    // GENERALISATION: every NEW replicated feature that must restore client-visible state on load needs its own such
    // processor keyed on this marker. Done: CkAttribute (Float/Byte/Integer). Pending: TagSet, Acceleration (CkPhysics),
    // AnimPlan, Inventory, and any future replicated container fragment.
    //
    // Never present during a Run_Capture (only ever stamped post-restore), so it never round-trips through a snapshot.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_Snapshot_JustRestored);
}

// --------------------------------------------------------------------------------------------------------------------
