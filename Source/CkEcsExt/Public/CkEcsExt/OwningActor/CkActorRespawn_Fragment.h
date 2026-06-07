#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Transient marker: "this restored bridged entity needs its actor respawned + re-bridged." Stamped by the
    // CkSnapshot load state machine AFTER Run_Restore (DoStamp_RespawnMarkers); consumed (removed) by
    // FProcessor_ActorRespawn inside the scheduler tick. Never present during a Run_Capture (only ever stamped
    // post-restore), so it never round-trips through a snapshot.
    CK_DEFINE_ECS_TAG(FTag_ActorRespawn_Pending);
}

// --------------------------------------------------------------------------------------------------------------------
