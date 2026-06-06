#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Velocity

static struct FVelocityRepHandlerRegistrar
{
    FVelocityRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Velocity::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(VelocityHandle))
                    { return; }

                    UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, New.Get<FCk_RepData_Velocity>().Value);
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    // Initial replication arrives as an Add, never a Change — an entity that spawns
                    // already moving would otherwise drop its starting velocity on clients.
                    auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(VelocityHandle))
                    { return; }

                    UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, Data.Get<FCk_RepData_Velocity>().Value);
                }
            });
    }
} GVelocityRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
