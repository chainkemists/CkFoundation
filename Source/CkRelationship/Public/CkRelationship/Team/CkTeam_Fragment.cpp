#include "CkTeam_Fragment.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Team

static struct FTeamRepHandlerRegistrar
{
    FTeamRepHandlerRegistrar()
    {
        const auto DoApplyTeam = [](FCk_Handle& Entity, ECk_Team_ID InTeamID)
        {
            if (UCk_Utils_Team_UE::Has(Entity))
            {
                auto TeamEntity = UCk_Utils_Team_UE::Cast(Entity);

                if (UCk_Utils_Team_UE::Get_IsAssignedTo(TeamEntity, InTeamID))
                { return; }

                UCk_Utils_Team_UE::Unassign(TeamEntity);
            }

            if (InTeamID == ECk_Team_ID::Unassigned)
            { return; }

            auto TeamEntity = UCk_Utils_Team_UE::Cast(Entity);
            UCk_Utils_Team_UE::Assign(TeamEntity, InTeamID);
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Team::StaticStruct(); },
            {
                .OnChange = [DoApplyTeam](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    DoApplyTeam(Entity, New.Get<FCk_RepData_Team>().Value);
                },
                .OnAdd = [DoApplyTeam](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyTeam(Entity, Data.Get<FCk_RepData_Team>().Value);
                }
            });
    }
} GTeamRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
