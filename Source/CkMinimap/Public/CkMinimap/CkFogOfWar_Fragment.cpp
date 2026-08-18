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
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_RepData_FogOfWar>({
                .Posture = ECk_Snapshot_Posture::Durable,
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
                // Waits on COMPOSITION and on nothing else. It must not wait on FTag_FogOfWar_NeedsSetup: a load
                // holds the entity out of every non-kernel processor's view until its payloads have applied, so a
                // handler that waits for Setup waits for something that cannot happen. Setup runs AFTER hydration
                // instead, and the ordering still works out — SetExplored is DEFERRED into
                // FFragment_FogOfWar_Requests, and FProcessor_FogOfWar_HandleRequests both RunAfter Setup and
                // excludes FTag_FogOfWar_NeedsSetup, so the grid is allocated before the saved cells are written.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_FogOfWar_UE::Has(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Payload = New.Get<FCk_RepData_FogOfWar>();
                    auto FogHandle = UCk_Utils_FogOfWar_UE::Cast(Entity);

                    UCk_Utils_FogOfWar_UE::Request_SetExplored(FogHandle,
                        FCk_Request_FogOfWar_SetExplored{Payload}, {});

                    return ECk_Persistence_ApplyResult::Applied;
                }});
        }
    };

    const FRegistrar GCkFogOfWarRepDataRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
