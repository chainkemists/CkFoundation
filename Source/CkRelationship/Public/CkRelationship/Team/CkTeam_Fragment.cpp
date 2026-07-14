#include "CkTeam_Fragment.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Team

static struct FTeamRepHandlerRegistrar
{
    FTeamRepHandlerRegistrar()
    {
        // Authority-safe applier: assigning the team from the payload is idempotent and host-safe, so the same
        // body serves both the net receive (Apply) and the load-path hydration (HydrationApply).
        const auto ApplyFn = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
        {
            if (NOT UCk_Utils_Team_UE::Has(Entity))
            { return ECk_Persistence_ApplyResult::NotReady; }

            auto TeamEntity = UCk_Utils_Team_UE::Cast(Entity);
            const auto TeamID = New.Get<FCk_RepData_Team>().Value;

            if (NOT UCk_Utils_Team_UE::Get_IsAssignedTo(TeamEntity, TeamID))
            { UCk_Utils_Team_UE::Assign(TeamEntity, TeamID); }

            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_Team>({
            // Produce-only capture (Phase 3A.4, [P1-R1]): mirror FProcessor_Team_Replicate's live-state build.
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT UCk_Utils_Team_UE::Has(Entity))
                { return {}; }
                return FInstancedStruct::Make(FCk_RepData_Team{Entity.Get<ck::FFragment_TeamInfo>().Get_TeamID()});
            },
            .SharedApply = ApplyFn});
    }
} GTeamRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
