#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkVoiceChatControlRelay_Actor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

// Voice CONTROL traffic - reliable, low-rate, stateful must-apply (a mute that gets lost is a
// privacy bug). It rides its OWN relay actor because amendment S1 bans reliable RPCs on the
// voice STREAM actor: a reliable attachment on that object would head-of-line-block the
// unreliable audio. A separate Iris object cannot.
UCLASS()
class CKVOICECHAT_API ACk_VoiceChatControlRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_VoiceChatControlRelay_UE);

public:
    // Full-set replace of this player's muted talkers (idempotent, ordering-proof). The player
    // is resolved server-side from the channel owner - clients are not trusted to self-identify.
    UFUNCTION(Server, Reliable)
    void
    Server_SetMutedTalkers(
        const TArray<FCk_Handle>& InMutedTalkers);

private:
    auto
    DoResolve_OwningPlayerState() const -> APlayerState*;
};

// --------------------------------------------------------------------------------------------------------------------
