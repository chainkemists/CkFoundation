#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Acceleration

static struct FAccelerationRepHandlerRegistrar
{
    FAccelerationRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Acceleration::StaticStruct(); },
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    auto AccelerationHandle = UCk_Utils_Acceleration_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(AccelerationHandle))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, New.Get<FCk_RepData_Acceleration>().Value);
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GAccelerationRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
