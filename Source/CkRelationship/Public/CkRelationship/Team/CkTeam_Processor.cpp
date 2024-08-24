#include "CkTeam_Processor.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

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
}
