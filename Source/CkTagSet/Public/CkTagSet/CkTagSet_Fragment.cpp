#include "CkTagSet_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for TagSet

static struct FTagSetRepHandlerRegistrar
{
    FTagSetRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_TagSet::StaticStruct(); },
            {
                // Stamps the sync fragment consumed by the TagSet SyncReplication processor (which
                // owns the actual diff/apply) — always Applied, the processor has its own gating.
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(
                        New.Get<FCk_RepData_TagSet>().Tags);
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GTagSetRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
