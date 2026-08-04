#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkVoiceChatRelay_Actor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

// Voice transport endpoint - one channel per player (group subsystem). Every RPC on this class
// is Unreliable BY CONTRACT and no Reliable RPC may ever be added to it: the fork's Iris stack
// marks unicast RPCs Ordered, so a reliable attachment on the same object head-of-line-blocks
// the voice stream. Bodies enqueue only; all validation and routing belongs to the Route
// processor. Transport contract: CkActorRelay/CLAUDE.md (the paced-stream clause).
UCLASS()
class CKVOICECHAT_API ACk_VoiceChatRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_VoiceChatRelay_UE);

public:
    // Talker -> server. The sender field is resolved server-side from the channel owner -
    // clients are not trusted to self-identify.
    UFUNCTION(Server, Unreliable)
    void
    Server_SendVoiceBundle(
        FCk_Handle InTalkerEntity,
        const TArray<uint8>& InPackedBundle);

    // Server -> one listener. Voice is disposable: an unresolvable talker drops the bundle and
    // counts it - never stashes (the stream simply resumes once the talker composes).
    UFUNCTION(Client, Unreliable)
    void
    Client_ReceiveVoiceBundle(
        FCk_Handle InTalkerEntity,
        const TArray<uint8>& InPackedBundle);

private:
    auto
    DoResolve_OwningPlayerState() const -> APlayerState*;
};

// --------------------------------------------------------------------------------------------------------------------
