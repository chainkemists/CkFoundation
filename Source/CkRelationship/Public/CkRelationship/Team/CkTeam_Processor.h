#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkRelationship/Team/CkTeam_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRELATIONSHIP_API FProcessor_TeamAssignedSignal_Setup : public TProcessor<
        FProcessor_TeamAssignedSignal_Setup,
        FTag_TeamListener_Setup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_TeamListener_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRELATIONSHIP_API FProcessor_OnTeamAssigned_Setup : public ck_exp::TProcessor<
        FProcessor_OnTeamAssigned_Setup,
        FCk_Handle_Team,
        ck::TReadOnly<FFragment_TeamInfo>,
        FTag_OnTeamAssigned_Setup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FTag_OnTeamAssigned_Setup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TeamInfo& InTeamInfo) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives a RESTORED Team to clients after a snapshot load. Team has no Replicate processor —
    // Assign pushes the container value inline — so post-restore the SEEDED container entry created
    // here is the only push (SetFragmentData marks it dirty for Iris). Also re-derives the per-ID
    // FTag_TeamID<> tag from the restored FFragment_TeamInfo value: Get_ID reads that tag, not the
    // fragment, and tags are not snapshotted. Pairs ck::FTag_Snapshot_JustRestored (shared
    // per-entity, never removed here) with the per-feature FTag_Team_RestoreReplicated done tag.
    // The view iterates the clean FFragment_TeamInfo and POINT-QUERIES the in_place marker (listing
    // it in the view would surface tombstones).
    class CKRELATIONSHIP_API FProcessor_Team_ReplicateOnRestore : public ck_exp::TProcessor<
        FProcessor_Team_ReplicateOnRestore,
        FCk_Handle_Team,
        ck::TReadOnly<FFragment_TeamInfo>,
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
            const FFragment_TeamInfo& InTeamInfo) const -> void;

    private:
        static auto
        DoRestoreTeamIDTag(
            HandleType& InHandle,
            ECk_Team_ID InTeamID) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
