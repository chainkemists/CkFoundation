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
                }
            });
    }
} GVelocityRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
