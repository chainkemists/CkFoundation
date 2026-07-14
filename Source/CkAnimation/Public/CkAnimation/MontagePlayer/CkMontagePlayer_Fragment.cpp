#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment (container-based replication for this feature)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // Register_* entry-point bodies

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for MontagePlayer (single-row, fragment-on-entity).

static struct FMontagePlayerRepHandlerRegistrar
{
    FMontagePlayerRepHandlerRegistrar()
    {
        // Authority-safe applier: DoDispatchReplicatedState from the payload is host-safe, so the same body serves
        // both the net receive (Apply) and the load-path hydration (HydrationApply). On the load path Old is always
        // unset, so IsFirstApplication is true — the correct "fresh state" semantics for a hydrated montage.
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

        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SharedApply<FCk_RepData_MontagePlayer>(
                // Produce is capture-only: builds the self-resident MontagePlayer container from live _State.
                [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                {
                    if (NOT Entity.Has<ck::FFragment_MontagePlayer_Current>())
                    { return {}; }
                    return FInstancedStruct::Make(FCk_RepData_MontagePlayer{Entity.Get<ck::FFragment_MontagePlayer_Current>().Get_State()});
                },
                ApplyFn);
    }
} GMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
