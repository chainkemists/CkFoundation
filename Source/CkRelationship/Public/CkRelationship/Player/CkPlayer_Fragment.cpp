#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Player

static struct FPlayerRepHandlerRegistrar
{
    FPlayerRepHandlerRegistrar()
    {
        // Authority-safe applier: assigning the player from the payload is idempotent and host-safe, so the same
        // body serves both the net receive (Apply) and the load-path hydration (HydrationApply).
        const auto ApplyFn = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
        {
            if (NOT UCk_Utils_Player_UE::Has(Entity))
            { return ECk_Persistence_ApplyResult::NotReady; }

            auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);
            const auto PlayerID = New.Get<FCk_RepData_Player>().Value;

            if (NOT UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, PlayerID))
            { UCk_Utils_Player_UE::Assign(PlayerEntity, PlayerID); }

            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_Player>({
            // Produce-only capture (Phase 3A.4, [P1-R1]): mirror FProcessor_Player_Replicate's live-state build.
            // Produce is capture-only.
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT UCk_Utils_Player_UE::Has(Entity))
                { return {}; }
                return FInstancedStruct::Make(FCk_RepData_Player{Entity.Get<ck::FFragment_PlayerInfo>().Get_PlayerID()});
            },
            .SharedApply = ApplyFn});
    }
} GPlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
