#include "CkVelocity_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (used by RegisterLazyTyped's default seed)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

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
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_Velocity>(
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(VelocityHandle))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    // (Phase 2 §2.6) The per-feature NeedsSetup apply-guard from 5eda3ac8a is retired: the late
                    // FGroup_Hydration dispatch + the ConstructedThisFrame defer (§2.4) + fire-gating (§2.5) now
                    // guarantee this apply runs AFTER the setup drain, so the applied value is final.
                    UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, New.Get<FCk_RepData_Velocity>().Value);
                    return ECk_RepFragment_ApplyResult::Applied;
                },
                // Restore re-seed of the self-resident Velocity container from live Current (mirrors the deleted
                // FProcessor_Velocity_ReplicateOnRestore). Default typed SeedContainer (no re-arm tag).
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_Velocity_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_Velocity{Entity.Get<ck::FFragment_Velocity_Current>().Get_CurrentVelocity()});
                }
            });
    }
} GVelocityRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
