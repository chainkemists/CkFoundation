#pragma once

#include "CkActorRelay/CkActorRelay_Actor.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkRenderTargetRelay_Actor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Every RPC rides the sending or receiving player's own reliable channel, so there is no
// resend/reorder logic anywhere in the module. Transport contract: CkRenderTarget/Claude.md.
UCLASS()
class CKRENDERTARGET_API ACk_RenderTargetRelay_UE : public ACk_ActorRelay_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_RenderTargetRelay_UE);

public:
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

    // Promotes this player's baseline server-side and switches them to delta streaming.
    UFUNCTION(Server, Reliable)
    void
    Server_AckFullSync(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        int32 InPayloadSeq);

    // Sent when the client lost instructions to ring wrap (or otherwise needs a baseline).
    UFUNCTION(Server, Reliable)
    void
    Server_RequestFullSync(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName);

    // Predicted batch, already applied on the authoring client. The batch's sender field is
    // overwritten server-side from the channel owner — clients are not trusted to self-identify.
    UFUNCTION(Server, Reliable)
    void
    Server_PushDrawBatch(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        const FCk_RenderTarget_InstructionBatch& InBatch);

    // v1 uploads are always FullSync — CkRenderTarget/Claude.md.
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
    auto
    DoResolve_OwningPlayerState() const -> APlayerState*;
};

// --------------------------------------------------------------------------------------------------------------------
