#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for MontagePlayer (single-row, fragment-on-entity).

static struct FMontagePlayerRepHandlerRegistrar
{
    FMontagePlayerRepHandlerRegistrar()
    {
        const auto DoApply = [](FCk_Handle& InEntity, const FCk_MontagePlayer_State& InNewState, bool InIsOnAdd) -> void
        {
            auto Handle = UCk_Utils_MontagePlayer_UE::Cast(InEntity);
            if (ck::Is_NOT_Valid(Handle))
            { return; }

            UCk_Utils_MontagePlayer_UE::DoDispatchReplicatedState(Handle, InNewState, InIsOnAdd);
        };

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_MontagePlayer::StaticStruct(); },
            {
                .OnChange = [DoApply](FCk_Handle& InEntity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    DoApply(InEntity, New.Get<FCk_RepData_MontagePlayer>().Value, /*IsOnAdd=*/false);
                },
                .OnAdd = [DoApply](FCk_Handle& InEntity, const FInstancedStruct& Data)
                {
                    DoApply(InEntity, Data.Get<FCk_RepData_MontagePlayer>().Value, /*IsOnAdd=*/true);
                }
            });
    }
} GMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
