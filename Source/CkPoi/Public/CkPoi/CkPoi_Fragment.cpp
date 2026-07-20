#include "CkPoi_Fragment.h"

#include "CkPoi/CkPoi_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKPOI_API, poi, ck::FFragment_Poi_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_poi_fragment
{
    // v3 save/load persistence handler for Poi. Save-only in phase 1 (Produce + HydrationApply, NO net Apply);
    // phase 4 upgrades this SAME registration to Register_NetAndSave_* — the payload already carries the
    // FCk_RepData_ name so old saves keep mapping (handlers key payloads by type path).
    //
    // COVERAGE (rebuild-model contract, not a bug): only recipe-rebuildable POIs persist —
    //   - POIs composed during their owner's Construct (game EntityScripts, UCk_Poi_EntityScript via level
    //     spawners or Create_Durable) are rebuilt on load and hydrated here.
    //   - Bare Create()'d hosts have no rebuild recipe: their payloads would orphan on load, so Produce
    //     additionally returns UNSET for TTL pings (FTag_Poi_TransientTtl) — transient BY DESIGN.
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_RepData_Poi>({
                // Save-capture: enable state + state tags. UNSET when the Poi feature is absent or the POI is a
                // deliberately-transient TTL ping.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT UCk_Utils_Poi_UE::Has(Entity))
                    { return {}; }

                    if (Entity.Has<ck::FTag_Poi_TransientTtl>())
                    { return {}; }

                    auto PoiHandle = UCk_Utils_Poi_UE::Cast(Entity);

                    auto Payload = FCk_RepData_Poi{};
                    Payload.Set_EnableDisable(UCk_Utils_Poi_UE::Get_EnableDisable(PoiHandle));
                    Payload.Set_StateTags(UCk_Utils_Poi_UE::Get_StateTags(PoiHandle));
                    return FInstancedStruct::Make(Payload);
                },
                // Load-path applier (authority-side). NotReady until the rebuilt entity has composed its Poi
                // fragments; then re-drive the runtime state through DEFERRED requests ONLY — the composite
                // SetStateTags request rides the request FIFO, guaranteeing the restored state lands AFTER any
                // state-seeding requests the replayed Construct enqueued. The NotReady gate precedes every request.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT UCk_Utils_Poi_UE::Has(Entity))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    if (Entity.Has<ck::FTag_Poi_NeedsSetup>())
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Payload = New.Get<FCk_RepData_Poi>();
                    auto PoiHandle = UCk_Utils_Poi_UE::Cast(Entity);

                    UCk_Utils_Poi_UE::Request_SetStateTags(PoiHandle,
                        FCk_Request_Poi_SetStateTags{Payload.Get_StateTags()});

                    if (UCk_Utils_Poi_UE::Get_EnableDisable(PoiHandle) != Payload.Get_EnableDisable())
                    {
                        UCk_Utils_Poi_UE::Request_EnableDisable(PoiHandle,
                            FCk_Request_Poi_EnableDisable{Payload.Get_EnableDisable()});
                    }

                    return ECk_Persistence_ApplyResult::Applied;
                }});
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkPoiRepDataRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
