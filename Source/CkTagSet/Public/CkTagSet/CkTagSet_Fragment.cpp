#include "CkTagSet_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (alias because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_TagSet = ck::FFragment_TagSet;
CK_REGISTER_SNAPSHOTABLE(FSnap_TagSet);

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
