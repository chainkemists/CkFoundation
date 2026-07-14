#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (used by RegisterLazyTyped's default seed)
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Acceleration

static struct FAccelerationRepHandlerRegistrar
{
    FAccelerationRepHandlerRegistrar()
    {
        // Authority-safe applier: Request_OverrideAcceleration from the payload is idempotent and host-safe, so the
        // same body serves both the net receive (Apply) and the load-path hydration (HydrationApply).
        const auto ApplyFn = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
        {
            auto AccelerationHandle = UCk_Utils_Acceleration_UE::Cast(Entity);
            if (ck::Is_NOT_Valid(AccelerationHandle))
            { return ECk_Persistence_ApplyResult::NotReady; }

            // (Phase 2 §2.6) The per-feature NeedsSetup apply-guard from 5eda3ac8a is retired: the late
            // FGroup_DeferredApply dispatch + the ConstructedThisFrame defer (§2.4) + fire-gating (§2.5) now
            // guarantee this apply runs AFTER the setup drain, so the applied value is final.
            UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, New.Get<FCk_RepData_Acceleration>().Value);
            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_Acceleration>({
                // Capture-only Produce of the self-resident Acceleration container from live Current.
                // HydrationApply reuses the net Apply; no explicit replication re-arm tag is added.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_Acceleration_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_Acceleration{Entity.Get<ck::FFragment_Acceleration_Current>().Get_CurrentAcceleration()});
                },
                .SharedApply = ApplyFn});
    }
} GAccelerationRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
