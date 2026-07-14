#include "CkAcceleration_Fragment.h"

#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (used by RegisterLazyTyped's default seed)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration (aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the name).

using FSnap_Acceleration_Params  = ck::FFragment_Acceleration_Params;
using FSnap_Acceleration_Current = ck::FFragment_Acceleration_Current;

CK_REGISTER_SNAPSHOTABLE(FSnap_Acceleration_Params);
CK_REGISTER_SNAPSHOTABLE(FSnap_Acceleration_Current);

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Acceleration

static struct FAccelerationRepHandlerRegistrar
{
    FAccelerationRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_Acceleration>(
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    auto AccelerationHandle = UCk_Utils_Acceleration_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(AccelerationHandle))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    // (Phase 2 §2.6) The per-feature NeedsSetup apply-guard from 5eda3ac8a is retired: the late
                    // FGroup_Hydration dispatch + the ConstructedThisFrame defer (§2.4) + fire-gating (§2.5) now
                    // guarantee this apply runs AFTER the setup drain, so the applied value is final.
                    UCk_Utils_Acceleration_UE::Request_OverrideAcceleration(AccelerationHandle, New.Get<FCk_RepData_Acceleration>().Value);
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Capture/oracle-only Produce of the self-resident Acceleration container from live Current
                // (the Model-A re-drive was retired in Phase 5). Default typed SeedContainer (no re-arm tag).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_Acceleration_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_Acceleration{Entity.Get<ck::FFragment_Acceleration_Current>().Get_CurrentAcceleration()});
                },
                .Transport = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GAccelerationRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
