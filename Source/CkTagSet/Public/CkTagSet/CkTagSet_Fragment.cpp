#include "CkTagSet_Fragment.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (RegisterLazyTyped default seed + custom SeedContainer)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

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
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_TagSet>(
            {
                // Stamps the sync fragment consumed by the TagSet SyncReplication processor (which
                // owns the actual diff/apply) — always Applied, the processor has its own gating.
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    Entity.AddOrGet<ck::FFragment_TagSet_SyncReplication>(
                        New.Get<FCk_RepData_TagSet>().Tags);
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Restore re-seed of the self-resident TagSet container from live tags (mirrors the deleted
                // FProcessor_TagSet_ReplicateOnRestore, whose payload shape came from FProcessor_TagSet_Replicate).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_TagSet>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_TagSet{Entity.Get<ck::FFragment_TagSet>().Get_Tags()});
                },
                // Custom seed: the typed add, then re-arm the ongoing replication trigger the TagSet Replicate
                // processor keys on (FProcessor_TagSet_Replicate MarkedDirtyBy FTag_TagSet_MayRequireReplication).
                .SeedContainer = [](FCk_Handle& Entity, const FInstancedStruct& Data) -> ECk_AddedOrNot
                {
                    const auto AddedOrNot = UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_TagSet>(
                        Entity, Data.Get<FCk_RepData_TagSet>());
                    if (AddedOrNot == ECk_AddedOrNot::NotAdded)
                    { return AddedOrNot; }

                    Entity.AddOrGet<ck::FTag_TagSet_MayRequireReplication>();
                    return AddedOrNot;
                },
                .Transport = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GTagSetRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
