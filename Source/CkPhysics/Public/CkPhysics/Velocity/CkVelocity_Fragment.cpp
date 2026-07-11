#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_Velocity_Params  = ck::FFragment_Velocity_Params;
using FSnap_Velocity_Current = ck::FFragment_Velocity_Current;
using FSnap_Velocity_MinMax  = ck::FFragment_Velocity_MinMax;

CK_REGISTER_SNAPSHOTABLE(FSnap_Velocity_Params);
CK_REGISTER_SNAPSHOTABLE(FSnap_Velocity_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_Velocity_MinMax);

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

                    // A pending Setup pass recomputes Current from Params AFTER this apply within the same frame,
                    // stomping the replicated value (and the authority never re-sends an unchanged value). Defer
                    // until the setup drain so the applied value is final.
                    if (Entity.Has<ck::FTag_Velocity_NeedsSetup>())
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, New.Get<FCk_RepData_Velocity>().Value);
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GVelocityRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
