#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (container-based replication for this feature)
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------

static struct FMontagePlayerRepHandlerRegistrar
{
    FMontagePlayerRepHandlerRegistrar()
    {
        // DoDispatchReplicatedState is host-safe, so one body serves both net receive and load hydration. On the
        // load path Old is always unset, so IsFirstApplication is true — correct "fresh state" for a hydrated montage.
        const auto ApplyFn = [](FCk_Handle& InEntity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
        {
            auto Handle = UCk_Utils_MontagePlayer_UE::Cast(InEntity);
            if (ck::Is_NOT_Valid(Handle))
            { return ECk_Persistence_ApplyResult::NotReady; }

            const auto IsFirstApplication = NOT Old.IsSet();
            UCk_Utils_MontagePlayer_UE::DoDispatchReplicatedState(
                Handle, New.Get<FCk_RepData_MontagePlayer>().Value, IsFirstApplication);
            return ECk_Persistence_ApplyResult::Applied;
        };

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_MontagePlayer>({
                .Posture = ECk_Snapshot_Posture::Durable,
                .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_MontagePlayer_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_MontagePlayer{Entity.Get<ck::FFragment_MontagePlayer_Current>().Get_State()});
                },
                .SharedApply = ApplyFn});
    }
} GMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
