#include "CkPlayer_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Player_ReplicateOnRestore);

namespace ck
{
    auto
        FProcessor_Player_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_PlayerInfo& InPlayerInfo) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        if (InHandle.Has<FTag_Player_RestoreReplicated>())
        { return; }

        // Re-derive BEFORE the driver gate: even a never-replicated restored entity needs its
        // per-ID tag back (Get_ID trips an ensure without it). Idempotent across retries.
        DoRestorePlayerIDTag(InHandle, InPlayerInfo.Get_PlayerID());

        // Driver not re-established yet (snapshot respawn pass) -> retry next tick (the shared
        // marker stays in place).
        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(InHandle))
        { return; }

        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Player>(
            InHandle, FCk_RepData_Player{InPlayerInfo.Get_PlayerID()});

        InHandle.Add<FTag_Player_RestoreReplicated>();
    }

    auto
        FProcessor_Player_ReplicateOnRestore::
        DoRestorePlayerIDTag(
            HandleType& InHandle,
            ECk_Player_ID InPlayerID)
        -> void
    {
        switch (InPlayerID)
        {
            case ECk_Player_ID::Zero:       { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Zero>>(); break; }
            case ECk_Player_ID::One:        { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::One>>(); break; }
            case ECk_Player_ID::Two:        { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Two>>(); break; }
            case ECk_Player_ID::Three:      { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Three>>(); break; }
            case ECk_Player_ID::Four:       { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Four>>(); break; }
            case ECk_Player_ID::Five:       { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Five>>(); break; }
            case ECk_Player_ID::Six:        { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Six>>(); break; }
            case ECk_Player_ID::Seven:      { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Seven>>(); break; }
            case ECk_Player_ID::Eight:      { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Eight>>(); break; }
            case ECk_Player_ID::Unassigned: { InHandle.AddOrGet<FTag_PlayerID<ECk_Player_ID::Unassigned>>(); break; }
            default:
            {
                CK_INVALID_ENUM(InPlayerID);
                break;
            }
        }
    }
}
