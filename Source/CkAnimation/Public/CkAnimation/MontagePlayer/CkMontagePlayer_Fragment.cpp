#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (RegisterLazyTyped default seed + custom SeedContainer)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_MontagePlayerParams = ck::FFragment_MontagePlayer_Params;
using FSnap_MontagePlayerCurrent = ck::FFragment_MontagePlayer_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_MontagePlayerParams);
CK_REGISTER_SNAPSHOTABLE(FSnap_MontagePlayerCurrent);

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for MontagePlayer (single-row, fragment-on-entity).

static struct FMontagePlayerRepHandlerRegistrar
{
    FMontagePlayerRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_MontagePlayer>(
            {
                .Apply = [](FCk_Handle& InEntity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    auto Handle = UCk_Utils_MontagePlayer_UE::Cast(InEntity);
                    if (ck::Is_NOT_Valid(Handle))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    const auto IsFirstApplication = NOT Old.IsSet();
                    UCk_Utils_MontagePlayer_UE::DoDispatchReplicatedState(
                        Handle, New.Get<FCk_RepData_MontagePlayer>().Value, IsFirstApplication);
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Restore re-seed of the self-resident MontagePlayer container from live _State (mirrors the deleted
                // FProcessor_MontagePlayer_ReplicateOnRestore).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_MontagePlayer_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_MontagePlayer{Entity.Get<ck::FFragment_MontagePlayer_Current>().Get_State()});
                },
                // Custom seed: typed add (self-gates on this entity's driver), then re-arm the Replicate trigger.
                .SeedContainer = [](FCk_Handle& Entity, const FInstancedStruct& Data) -> ECk_AddedOrNot
                {
                    const auto AddedOrNot = UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_MontagePlayer>(
                        Entity, Data.Get<FCk_RepData_MontagePlayer>());
                    if (AddedOrNot == ECk_AddedOrNot::NotAdded)
                    { return AddedOrNot; }

                    Entity.AddOrGet<ck::FTag_MontagePlayer_MayRequireReplication>();
                    return AddedOrNot;
                },
                .Transport = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
