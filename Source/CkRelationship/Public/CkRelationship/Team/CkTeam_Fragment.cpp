#include "CkTeam_Fragment.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (alias because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_TeamInfo = ck::FFragment_TeamInfo;
CK_REGISTER_SNAPSHOTABLE(FSnap_TeamInfo);

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Team

static struct FTeamRepHandlerRegistrar
{
    FTeamRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
        []() -> UScriptStruct* { return FCk_RepData_Team::StaticStruct(); },
        {
            .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
            {
                if (NOT UCk_Utils_Team_UE::Has(Entity))
                { return ECk_RepFragment_ApplyResult::NotReady; }

                auto TeamEntity = UCk_Utils_Team_UE::Cast(Entity);
                const auto TeamID = New.Get<FCk_RepData_Team>().Value;

                if (NOT UCk_Utils_Team_UE::Get_IsAssignedTo(TeamEntity, TeamID))
                { UCk_Utils_Team_UE::Assign(TeamEntity, TeamID); }

                return ECk_RepFragment_ApplyResult::Applied;
            },
            // Produce-only capture (Phase 3A.4, [P1-R1]): mirror FProcessor_Team_Replicate's live-state build. NO
            // SeedContainer — the live FProcessor_Team_ReplicateOnRestore still seeds under Model A (double-seed guard).
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT UCk_Utils_Team_UE::Has(Entity))
                { return {}; }
                return FInstancedStruct::Make(FCk_RepData_Team{Entity.Get<ck::FFragment_TeamInfo>().Get_TeamID()});
            },
            .Transport = ECk_PersistenceTransport::NetAndSave
        });
    }
} GTeamRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
