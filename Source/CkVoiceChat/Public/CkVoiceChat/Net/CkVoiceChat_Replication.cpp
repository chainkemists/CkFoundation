#include "CkVoiceChat/Net/CkVoiceChat_RepData.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h"

// --------------------------------------------------------------------------------------------------------------------
// Net-only control-plane handler: voice is runtime net state, nothing is ever saved. The server
// pushes the container at every mutation site (idx assignment + membership handlers); this
// applier runs deferred on clients with the standard Apply/NotReady contract.

static struct FVoiceChatRepHandlerRegistrar
{
    FVoiceChatRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_VoiceChat>({
            .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
            {
                return UCk_Utils_VoiceChannel_UE::Apply_ReplicatedControlPlane(
                    Entity, New.Get<FCk_RepData_VoiceChat>());
            },
        });
    }
} GVoiceChatRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
