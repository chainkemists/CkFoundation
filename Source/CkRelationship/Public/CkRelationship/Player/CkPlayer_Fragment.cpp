#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (alias because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_PlayerInfo = ck::FFragment_PlayerInfo;
CK_REGISTER_SNAPSHOTABLE(FSnap_PlayerInfo);

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Player

static struct FPlayerRepHandlerRegistrar
{
    FPlayerRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
        []() -> UScriptStruct* { return FCk_RepData_Player::StaticStruct(); },
        {
            .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
            {
                if (NOT UCk_Utils_Player_UE::Has(Entity))
                { return ECk_RepFragment_ApplyResult::NotReady; }

                auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);
                const auto PlayerID = New.Get<FCk_RepData_Player>().Value;

                if (NOT UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, PlayerID))
                { UCk_Utils_Player_UE::Assign(PlayerEntity, PlayerID); }

                return ECk_RepFragment_ApplyResult::Applied;
            }
        });
    }
} GPlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
