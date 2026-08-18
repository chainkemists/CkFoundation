#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (used by RegisterLazyTyped's default seed)
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------

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

            UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, New.Get<FCk_RepData_Acceleration>().Value, {});
            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_Acceleration>({
                .Posture = ECk_Snapshot_Posture::Durable,
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
