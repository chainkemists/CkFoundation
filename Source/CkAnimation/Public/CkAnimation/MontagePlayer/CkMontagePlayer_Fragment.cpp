#include "CkMontagePlayer_Fragment.h"

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

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
