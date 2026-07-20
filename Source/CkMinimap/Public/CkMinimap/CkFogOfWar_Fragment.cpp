#include "CkFogOfWar_Fragment.h"

#include "CkMinimap/CkFogOfWar_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKMINIMAP_API, fog_of_war, ck::FFragment_FogOfWar_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_fog_of_war_fragment
{
    // v3 save/load persistence handler for FogOfWar. Save-only in phase 1 (Produce + HydrationApply, NO net Apply);
    // phase 4 upgrades this SAME registration to Register_NetAndSave_* for co-op shared exploration — the payload
    // already carries the FCk_RepData_ name so old saves keep mapping (handlers key payloads by type path).
    //
    // COVERAGE (rebuild-model contract, not a bug): only recipe-rebuildable fog persists — a FogOfWar grid composed
    // during its entity's Construct (game EntityScripts) is rebuilt on load and hydrated here. Fog Add'ed onto a
    // bare-created entity has no rebuild recipe: its payload orphans on load.
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_RepData_FogOfWar>({
                // Save-capture: the packed explored grid. UNSET when the FogOfWar feature is absent or its grid
                // never allocated (invalid bounds / cell budget blown at Setup — nothing meaningful to restore).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_FogOfWar_UE::Has(Entity))
                    { return {}; }

                    auto FogHandle = UCk_Utils_FogOfWar_UE::Cast(Entity);

                    const auto CellCounts = UCk_Utils_FogOfWar_UE::Get_CellCounts(FogHandle);

                    if (CellCounts.X <= 0 || CellCounts.Y <= 0)
                    { return {}; }

                    return FInstancedStruct::Make(UCk_Utils_FogOfWar_UE::Get_ExploredData(FogHandle));
                },
                // Load-path applier (authority-side). NotReady until the rebuilt entity has composed its FogOfWar
                // fragments AND Setup allocated the grid; then re-drive the explored state through a DEFERRED
                // request ONLY — SetExplored rides the request FIFO, guaranteeing the restored cells land AFTER
                // any reveals the replayed Construct enqueued (its UNION semantics keep that ordering safe).
                // Cell-count mismatches (map/bounds changed since the save) are detected by the PROCESSOR handling
                // SetExplored — the restore is dropped loudly there and the fresh grid is kept.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_FogOfWar_UE::Has(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    if (Entity.Has<ck::FTag_FogOfWar_NeedsSetup>())
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Payload = New.Get<FCk_RepData_FogOfWar>();
                    auto FogHandle = UCk_Utils_FogOfWar_UE::Cast(Entity);

                    UCk_Utils_FogOfWar_UE::Request_SetExplored(FogHandle,
                        FCk_Request_FogOfWar_SetExplored{Payload});

                    return ECk_Persistence_ApplyResult::Applied;
                }});
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkFogOfWarRepDataRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
