#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Player

static struct FPlayerRepHandlerRegistrar
{
    FPlayerRepHandlerRegistrar()
    {
        const auto DoApplyPlayer = [](FCk_Handle& Entity, ECk_Player_ID InPlayerID)
        {
            if (UCk_Utils_Player_UE::Has(Entity))
            {
                auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);

                if (UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, InPlayerID))
                { return; }

                UCk_Utils_Player_UE::Unassign(PlayerEntity);
            }

            if (InPlayerID == ECk_Player_ID::Unassigned)
            { return; }

            auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);
            UCk_Utils_Player_UE::Assign(PlayerEntity, InPlayerID);
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Player::StaticStruct(); },
            {
                .OnChange = [DoApplyPlayer](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    DoApplyPlayer(Entity, New.Get<FCk_RepData_Player>().Value);
                },
                .OnAdd = [DoApplyPlayer](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    DoApplyPlayer(Entity, Data.Get<FCk_RepData_Player>().Value);
                }
            });
    }
} GPlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
