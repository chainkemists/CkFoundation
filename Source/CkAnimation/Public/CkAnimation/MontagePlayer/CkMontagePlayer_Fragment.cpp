#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for MontagePlayer (single-row, fragment-on-entity).

static struct FMontagePlayerRepHandlerRegistrar
{
    FMontagePlayerRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_MontagePlayer::StaticStruct(); },
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
                }
            });
    }
} GMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
