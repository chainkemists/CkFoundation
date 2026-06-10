#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkRelationship/Player/CkPlayer_Fragment.h"
#include "CkRelationship/Player/CkPlayer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives a RESTORED Player to clients after a snapshot load. Player has no Replicate
    // processor — Assign pushes the container value inline — so post-restore the SEEDED container
    // entry created here is the only push (SetFragmentData marks it dirty for Iris). Also
    // re-derives the per-ID FTag_PlayerID<> tag from the restored FFragment_PlayerInfo value:
    // Get_ID reads that tag, not the fragment, and tags are not snapshotted. Pairs
    // ck::FTag_Snapshot_JustRestored (shared per-entity, never removed here) with the per-feature
    // FTag_Player_RestoreReplicated done tag. The view iterates the clean FFragment_PlayerInfo and
    // POINT-QUERIES the in_place marker (listing it in the view would surface tombstones).
    class CKRELATIONSHIP_API FProcessor_Player_ReplicateOnRestore : public ck_exp::TProcessor<
        FProcessor_Player_ReplicateOnRestore,
        FCk_Handle_Player,
        ck::TReadOnly<FFragment_PlayerInfo>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PlayerInfo& InPlayerInfo) const -> void;

    private:
        static auto
        DoRestorePlayerIDTag(
            HandleType& InHandle,
            ECk_Player_ID InPlayerID) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
