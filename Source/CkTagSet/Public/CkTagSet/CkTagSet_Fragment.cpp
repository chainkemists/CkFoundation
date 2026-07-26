#include "CkTagSet_Fragment.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (container-based replication for this feature)
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------

static struct FTagSetRepHandlerRegistrar
{
    FTagSetRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_TagSet>({
                // Payload shape matches FProcessor_TagSet_Replicate.
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_TagSet>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_TagSet{Entity.Get<ck::FFragment_TagSet>().Get_Tags()});
                },
                // Stamps the sync fragment consumed by the TagSet SyncReplication processor (which
                // owns the actual diff/apply) — always Applied, the processor has its own gating.
                .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    const auto& SavedTags = New.Get<FCk_RepData_TagSet>().Tags;
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(SavedTags);
                    return ECk_Persistence_ApplyResult::Applied;
                },
                // Set_Tags, NOT Request_AddTags: Construct re-seeds default tags, so an ADD would
                // resurrect a runtime-removed tag. See CkTagSet/CLAUDE.md § Persistence.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    if (NOT Entity.Has<ck::FFragment_TagSet>())
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& SavedTags = New.Get<FCk_RepData_TagSet>().Tags;
                    Entity.Get<ck::FFragment_TagSet>().Set_Tags(SavedTags);
                    Entity.AddOrGet<ck::FTag_TagSet_MayRequireReplication>();
                    return ECk_Persistence_ApplyResult::Applied;
                }});
    }
} GTagSetRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
