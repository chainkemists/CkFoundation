#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkRenderTargetRelay_Actor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Per-player RPC relay for RenderTarget pixel streams (channel B).
//
// Server→client pixel payloads ride reliable unicast Client RPCs on the receiving player's
// channel (reliable ⇒ ordered per connection ⇒ no resend/reorder logic); client→server traffic
// (acks, baseline requests, and authoring-client pushes) rides Server RPCs on the sending
// player's own channel. RPC bodies only enqueue ECS requests — the net-receive path stays pure
// bookkeeping (framework rule). Cross-machine handle resolution is transparent:
// FCk_Handle::NetSerialize carries the entity's ReplicationDriver, so InOwnerEntity arrives
// already pointing at the local world's owner entity.
UCLASS()
class CKRENDERTARGET_API ACk_RenderTargetRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_RenderTargetRelay_UE);

public:
    // One chunk of a compressed pixel payload (FullSync baseline or block-delta). Chunks of one
    // payload arrive in idx order on the reliable channel; the receiver reassembles by count.
    // InInstructionWatermark is the authoring side's applied-batch seq at capture time — on
    // FullSync apply the receiver drops instruction batches at or below it (already baked in).
    UFUNCTION(Client, Reliable)
    void
    Client_ReceivePixelChunk(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        ECk_RenderTarget_PixelPayloadKind InKind,
        int32 InPayloadSeq,
        int32 InChunkIdx,
        int32 InNumChunks,
        int32 InUncompressedSize,
        FIntPoint InSize,
        int32 InInstructionWatermark,
        const TArray<uint8>& InBytes);

    // Client confirms a FullSync payload fully applied — the server promotes this player's
    // baseline and switches them to delta streaming. Deltas need no ack (reliable channel).
    UFUNCTION(Server, Reliable)
    void
    Server_AckFullSync(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        int32 InPayloadSeq);

    // Client lost instructions to ring wrap (or otherwise needs a baseline) — the server queues
    // a FullSync stream for this player.
    UFUNCTION(Server, Reliable)
    void
    Server_RequestFullSync(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName);

    // Authoring client pushes a predicted draw batch (already applied locally). The server
    // validates the _ClientAuthoring gate, applies, assigns its own seq, and republishes through
    // the instruction ring with the sender stamped for echo suppression. The batch's sender field
    // is overwritten server-side from the channel owner — clients are not trusted to self-identify.
    UFUNCTION(Server, Reliable)
    void
    Server_PushDrawBatch(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        const FCk_RenderTarget_InstructionBatch& InBatch);

    // Authoring client pushes one chunk of a pixel upload. v1 uploads are always FullSync: a
    // delta upload would diff against the uploader's own history, which may have diverged from
    // the server snapshot. Same shape as Client_ReceivePixelChunk, opposite direction.
    UFUNCTION(Server, Reliable)
    void
    Server_PushPixelChunk(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        ECk_RenderTarget_PixelPayloadKind InKind,
        int32 InPayloadSeq,
        int32 InChunkIdx,
        int32 InNumChunks,
        int32 InUncompressedSize,
        FIntPoint InSize,
        int32 InInstructionWatermark,
        const TArray<uint8>& InBytes);

private:
    // The PlayerState that owns this channel — Server RPC handlers use it to identify the
    // requesting player (channels are player-owned; the owner chain is PlayerController-rooted).
    auto
    DoResolve_OwningPlayerState() const -> APlayerState*;
};

// --------------------------------------------------------------------------------------------------------------------
