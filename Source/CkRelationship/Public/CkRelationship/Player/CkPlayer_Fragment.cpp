#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Player

static struct FPlayerRepHandlerRegistrar
{
    FPlayerRepHandlerRegistrar()
    {
        // Authority-safe applier: assigning the player from the payload is idempotent and host-safe, so the same
        // body serves both the net receive (Apply) and the load-path hydration (HydrationApply).
        const auto ApplyFn = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
        {
            if (NOT UCk_Utils_Player_UE::Has(Entity))
            { return ECk_RepFragment_ApplyResult::NotReady; }

            auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);
            const auto PlayerID = New.Get<FCk_RepData_Player>().Value;

            if (NOT UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, PlayerID))
            { UCk_Utils_Player_UE::Assign(PlayerEntity, PlayerID); }

            return ECk_RepFragment_ApplyResult::Applied;
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_Player>(
        {
            .Apply = ApplyFn,
            .HydrationApply = ApplyFn,
            // Produce-only capture (Phase 3A.4, [P1-R1]): mirror FProcessor_Player_Replicate's live-state build. NO
            // SeedContainer — Produce is capture/oracle-only; the Model-A re-drive was retired in Phase 5.
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT UCk_Utils_Player_UE::Has(Entity))
                { return {}; }
                return FInstancedStruct::Make(FCk_RepData_Player{Entity.Get<ck::FFragment_PlayerInfo>().Get_PlayerID()});
            },
        });
    }
} GPlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
