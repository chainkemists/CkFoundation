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
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(VelocityHandle))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, New.Get<FCk_RepData_Velocity>().Value);
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GVelocityRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
