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
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    auto AccelerationHandle = UCk_Utils_Acceleration_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(AccelerationHandle))
                    { return; }

                    UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, New.Get<FCk_RepData_Acceleration>().Value);
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    // Initial replication arrives as an Add, never a Change — an entity that spawns
                    // with a starting acceleration would otherwise drop it on clients.
                    auto AccelerationHandle = UCk_Utils_Acceleration_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(AccelerationHandle))
                    { return; }

                    UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, Data.Get<FCk_RepData_Acceleration>().Value);
                }
            });
    }
} GAccelerationRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
