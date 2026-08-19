#include "CkSnapshot/Subsystem/CkSnapshot_LoadState.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_load_state
{
    // Wire-only, and Session with it. The fact describes a load in flight; capturing it would put "a load was
    // happening" into the saved world, and hydrating it would tell the next load that the previous one had
    // already finished.
    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_SnapshotLoadState>({
                .Posture = ECk_Snapshot_Posture::Session,
                .NetApply = [](FCk_Handle& InEntity, const FInstancedStruct& InNew,
                               const TOptional<FInstancedStruct>& /*InOld*/) -> ECk_Persistence_ApplyResult
                {
                    const auto& Data = InNew.Get<FCk_RepData_SnapshotLoadState>();

                    // No composition to wait on: the channel entity carries this fragment BECAUSE the payload
                    // arrived, which is the whole reason a client can read the fact without the loader existing
                    // on its side.
                    auto& State = InEntity.AddOrGet<ck::FFragment_Snapshot_LoadState>();
                    State.Set_LoadEpoch(Data.LoadEpoch);
                    State.Set_ReadyToResume(Data.ReadyToResume);

                    return ECk_Persistence_ApplyResult::Applied;
                }});
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkSnapshotLoadStateRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
