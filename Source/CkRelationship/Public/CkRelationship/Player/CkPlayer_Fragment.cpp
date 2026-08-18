#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

static struct FPlayerRepHandlerRegistrar
{
    FPlayerRepHandlerRegistrar()
    {
        // Idempotent and host-safe, so one body serves both net receive (Apply) and load hydration.
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
            .Posture = ECk_Snapshot_Posture::Durable,
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
