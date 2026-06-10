#include "CkTeam_Processor.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_TeamAssignedSignal_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_OnTeamAssigned_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Team_ReplicateOnRestore);

namespace
ck
{
    auto
        FProcessor_TeamAssignedSignal_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        if (UCk_Utils_Team_UE::Has(InHandle))
        {
            const auto ThisTeamHandle = UCk_Utils_Team_UE::Cast(InHandle);

            InHandle.View<ck::FFragment_TeamInfo>().ForEach([&](EntityType InTeamEntity, const ck::FFragment_TeamInfo& InTeamInfo)
            {
                const auto TeamHandle = UCk_Utils_Team_UE::Cast(MakeHandle(InTeamEntity, InHandle));

                if (UCk_Utils_Team_UE::Get_IsSame(ThisTeamHandle, TeamHandle))
                {
                    UUtils_Signal_TeamAssigned_OnSameTeam::Broadcast(ThisTeamHandle, MakePayload(TeamHandle, InTeamInfo.Get_TeamID()));
                }
                else
                {
                    UUtils_Signal_TeamAssigned_OnOpposingTeam::Broadcast(ThisTeamHandle, MakePayload(TeamHandle, InTeamInfo.Get_TeamID()));
                }
            });
        }
        else
        {
            InHandle.View<ck::FFragment_TeamInfo>().ForEach([&](EntityType InTeamEntity, const ck::FFragment_TeamInfo& InTeamInfo)
            {
                const auto TeamHandle = UCk_Utils_Team_UE::Cast(MakeHandle(InTeamEntity, InHandle));
                UUtils_Signal_TeamAssigned::Broadcast(InHandle, MakePayload(TeamHandle, InTeamInfo.Get_TeamID()));
            });
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_OnTeamAssigned_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_TeamInfo& InTeamInfo) const
        -> void
    {
        InHandle.View<ck::FFragment_TeamInfo, FTag_TeamListener>().ForEach([&](EntityType InTeamEntity, const ck::FFragment_TeamInfo& InTeamInfo)
        {
            const auto TeamHandle = UCk_Utils_Team_UE::Cast(MakeHandle(InTeamEntity, InHandle));

            if (UCk_Utils_Team_UE::Get_IsSame(InHandle, TeamHandle))
            {
                UUtils_Signal_TeamAssigned_OnSameTeam::Broadcast(TeamHandle, MakePayload(InHandle, InTeamInfo.Get_TeamID()));
            }
            else
            {
                UUtils_Signal_TeamAssigned_OnOpposingTeam::Broadcast(TeamHandle, MakePayload(InHandle, InTeamInfo.Get_TeamID()));
            }
        });

        InHandle.View<FTag_TeamListener>().ForEach([&](EntityType InOtherEntity)
        {
            const auto OtherHandle = MakeHandle(InOtherEntity, InHandle);
            UUtils_Signal_TeamAssigned::Broadcast(OtherHandle, MakePayload(InHandle, InTeamInfo.Get_TeamID()));
        });

        InHandle.Remove<FTag_OnTeamAssigned_Setup>();
    }

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    auto
        FProcessor_Team_ReplicateOnRestore::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_TeamInfo& InTeamInfo) const
        -> void
    {
        if (NOT InHandle.Has<FTag_Snapshot_JustRestored>())
        { return; }

        if (InHandle.Has<FTag_Team_RestoreReplicated>())
        { return; }

        // Re-derive BEFORE the driver gate: even a never-replicated restored entity needs its
        // per-ID tag back (Get_ID trips an ensure without it). Idempotent across retries.
        DoRestoreTeamIDTag(InHandle, InTeamInfo.Get_TeamID());

        // Driver not re-established yet (snapshot respawn pass) -> retry next tick (the shared
        // marker stays in place).
        if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(InHandle))
        { return; }

        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_Team>(
            InHandle, FCk_RepData_Team{InTeamInfo.Get_TeamID()});

        InHandle.Add<FTag_Team_RestoreReplicated>();
    }

    auto
        FProcessor_Team_ReplicateOnRestore::
        DoRestoreTeamIDTag(
            HandleType& InHandle,
            ECk_Team_ID InTeamID)
        -> void
    {
        switch (InTeamID)
        {
            case ECk_Team_ID::Zero:       { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Zero>>(); break; }
            case ECk_Team_ID::One:        { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::One>>(); break; }
            case ECk_Team_ID::Two:        { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Two>>(); break; }
            case ECk_Team_ID::Three:      { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Three>>(); break; }
            case ECk_Team_ID::Four:       { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Four>>(); break; }
            case ECk_Team_ID::Five:       { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Five>>(); break; }
            case ECk_Team_ID::Six:        { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Six>>(); break; }
            case ECk_Team_ID::Seven:      { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Seven>>(); break; }
            case ECk_Team_ID::Eight:      { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Eight>>(); break; }
            case ECk_Team_ID::Nine:       { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Nine>>(); break; }
            case ECk_Team_ID::Ten:        { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Ten>>(); break; }
            case ECk_Team_ID::Eleven:     { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Eleven>>(); break; }
            case ECk_Team_ID::Twelve:     { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Twelve>>(); break; }
            case ECk_Team_ID::Unassigned: { InHandle.AddOrGet<FTag_TeamID<ECk_Team_ID::Unassigned>>(); break; }
            default:
            {
                CK_INVALID_ENUM(InTeamID);
                break;
            }
        }
    }
}
