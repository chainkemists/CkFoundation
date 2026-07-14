#include "CkTagSet_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (RegisterLazyTyped default seed + custom SeedContainer)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for TagSet

static struct FTagSetRepHandlerRegistrar
{
    FTagSetRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_TagSet>(
            {
                // Stamps the sync fragment consumed by the TagSet SyncReplication processor (which
                // owns the actual diff/apply) — always Applied, the processor has its own gating.
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    const auto& SavedTags = New.Get<FCk_RepData_TagSet>().Tags;
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(SavedTags);
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Save-load hydration (authority-side, Phase 4B): the sync-fragment stamp in Apply only drains on
                // CLIENTS (FProcessor_TagSet_SyncReplication is ClientOnly), so on the loading AUTHORITY it would
                // never restore the server's tags. REPLACE the authority tag set directly (Set_Tags, not
                // Request_AddTags — Construct re-seeds default tags, so ADD would resurrect a runtime-removed tag)
                // and re-arm replication so post-load clients converge. Mirrors the client drain's Set_Tags
                // (CkTagSet_Processor.cpp:160) + the MayRequireReplication arm.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    if (NOT Entity.Has<ck::FFragment_TagSet>())
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    const auto& SavedTags = New.Get<FCk_RepData_TagSet>().Tags;
                    Entity.Get<ck::FFragment_TagSet>().Set_Tags(SavedTags);
                    Entity.AddOrGet<ck::FTag_TagSet_MayRequireReplication>();
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Capture/oracle-only Produce of the self-resident TagSet container from live tags
                // (the Model-A re-drive was retired in Phase 5; payload shape matches FProcessor_TagSet_Replicate).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_TagSet>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_TagSet{Entity.Get<ck::FFragment_TagSet>().Get_Tags()});
                },
            });
    }
} GTagSetRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
