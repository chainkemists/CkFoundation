#include "CkRenderTargetRelay_Actor.h"

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Utils.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    Client_ReceivePixelChunk_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        ECk_RenderTarget_PixelPayloadKind InKind,
        int32 InPayloadSeq,
        int32 InChunkIdx,
        int32 InNumChunks,
        int32 InUncompressedSize,
        FIntPoint InSize,
        int32 InInstructionWatermark,
        const TArray<uint8>& InBytes)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::render_target::Warning(
            TEXT("Client_ReceivePixelChunk: owner entity did not resolve (driver missing or entity destroyed). ")
            TEXT("Dropping chunk [{}/{}] of payload [{}] for sync [{}]."),
            InChunkIdx + 1, InNumChunks, InPayloadSeq, InSyncName);
        return;
    }

    auto Chunk = ck::FCk_RenderTarget_PixelChunk{}
        .Set_Kind(InKind)
        .Set_PayloadSeq(InPayloadSeq)
        .Set_ChunkIdx(InChunkIdx)
        .Set_NumChunks(InNumChunks)
        .Set_UncompressedSize(InUncompressedSize)
        .Set_Size(InSize)
        .Set_InstructionWatermark(InInstructionWatermark)
        .Set_Bytes(InBytes);

    auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, InSyncName);

    if (ck::IsValid(SyncEntity))
    {
        SyncEntity.AddOrGet<ck::FFragment_RenderTarget_ChunkInbox>().Get_Chunks().Emplace(MoveTemp(Chunk));
        return;
    }

    // The sync child hasn't composed yet (chunk raced construction) — stash on the owner;
    // FProcessor_RenderTarget_FlushOwnerChunkStash routes it once the child exists.
    InOwnerEntity.AddOrGet<ck::FFragment_RenderTarget_OwnerChunkStash>().Get_StashedChunks().Emplace(
        ck::FCk_RenderTarget_StashedChunk{InSyncName, MoveTemp(Chunk)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    Server_AckFullSync_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        int32 InPayloadSeq)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::render_target::Warning(TEXT("Server_AckFullSync: owner entity did not resolve. Dropping ack of payload [{}] for sync [{}]."),
            InPayloadSeq, InSyncName);
        return;
    }

    auto* RequestingPlayer = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(RequestingPlayer))
    {
        ck::render_target::Warning(TEXT("Server_AckFullSync: could not resolve the requesting player from the channel owner chain."));
        return;
    }

    auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, InSyncName);

    if (ck::Is_NOT_Valid(SyncEntity))
    {
        ck::render_target::Warning(TEXT("Server_AckFullSync: sync [{}] not found on owner [{}]."),
            InSyncName, InOwnerEntity);
        return;
    }

    SyncEntity.AddOrGet<ck::FFragment_RenderTarget_HostInbox>().Get_Requests().Emplace(
        ck::FCk_RenderTarget_StreamRequest{
            ck::ECk_RenderTarget_StreamRequestKind::AckFullSync, RequestingPlayer, InPayloadSeq});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    Server_RequestFullSync_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::render_target::Warning(TEXT("Server_RequestFullSync: owner entity did not resolve. Dropping request for sync [{}]."),
            InSyncName);
        return;
    }

    auto* RequestingPlayer = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(RequestingPlayer))
    {
        ck::render_target::Warning(TEXT("Server_RequestFullSync: could not resolve the requesting player from the channel owner chain."));
        return;
    }

    auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, InSyncName);

    if (ck::Is_NOT_Valid(SyncEntity))
    {
        ck::render_target::Warning(TEXT("Server_RequestFullSync: sync [{}] not found on owner [{}]."),
            InSyncName, InOwnerEntity);
        return;
    }

    SyncEntity.AddOrGet<ck::FFragment_RenderTarget_HostInbox>().Get_Requests().Emplace(
        ck::FCk_RenderTarget_StreamRequest{
            ck::ECk_RenderTarget_StreamRequestKind::RequestFullSync, RequestingPlayer, 0});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    Server_PushDrawBatch_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        const FCk_RenderTarget_InstructionBatch& InBatch)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::render_target::Warning(TEXT("Server_PushDrawBatch: owner entity did not resolve. Dropping [{}] cmd(s) for sync [{}]."),
            InBatch.Get_Cmds().Num(), InSyncName);
        return;
    }

    auto* RequestingPlayer = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(RequestingPlayer))
    {
        ck::render_target::Warning(TEXT("Server_PushDrawBatch: could not resolve the requesting player from the channel owner chain."));
        return;
    }

    auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, InSyncName);

    if (ck::Is_NOT_Valid(SyncEntity))
    {
        ck::render_target::Warning(TEXT("Server_PushDrawBatch: sync [{}] not found on owner [{}]."),
            InSyncName, InOwnerEntity);
        return;
    }

    // Stamp the sender server-side — clients are not trusted to self-identify (echo suppression
    // and upload rate limits key off this).
    auto Batch = InBatch;
    Batch.Set_Sender(RequestingPlayer);

    SyncEntity.AddOrGet<ck::FFragment_RenderTarget_ServerIngressBatches>().Get_Batches().Emplace(MoveTemp(Batch));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    Server_PushPixelChunk_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InSyncName,
        ECk_RenderTarget_PixelPayloadKind InKind,
        int32 InPayloadSeq,
        int32 InChunkIdx,
        int32 InNumChunks,
        int32 InUncompressedSize,
        FIntPoint InSize,
        int32 InInstructionWatermark,
        const TArray<uint8>& InBytes)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::render_target::Warning(TEXT("Server_PushPixelChunk: owner entity did not resolve. Dropping chunk [{}/{}] for sync [{}]."),
            InChunkIdx + 1, InNumChunks, InSyncName);
        return;
    }

    auto* RequestingPlayer = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(RequestingPlayer))
    {
        ck::render_target::Warning(TEXT("Server_PushPixelChunk: could not resolve the requesting player from the channel owner chain."));
        return;
    }

    auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, InSyncName);

    if (ck::Is_NOT_Valid(SyncEntity))
    {
        ck::render_target::Warning(TEXT("Server_PushPixelChunk: sync [{}] not found on owner [{}]."),
            InSyncName, InOwnerEntity);
        return;
    }

    auto Chunk = ck::FCk_RenderTarget_PixelChunk{}
        .Set_Kind(InKind)
        .Set_PayloadSeq(InPayloadSeq)
        .Set_ChunkIdx(InChunkIdx)
        .Set_NumChunks(InNumChunks)
        .Set_UncompressedSize(InUncompressedSize)
        .Set_Size(InSize)
        .Set_InstructionWatermark(InInstructionWatermark)
        .Set_Bytes(InBytes);

    SyncEntity.AddOrGet<ck::FFragment_RenderTarget_UploadAssembly>().Get_Inbox().Emplace(
        ck::FCk_RenderTarget_UploadChunk{RequestingPlayer, MoveTemp(Chunk)});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_RenderTargetRelay_UE::
    DoResolve_OwningPlayerState() const
    -> APlayerState*
{
    for (auto* Outer = GetOwner(); Outer != nullptr; Outer = Outer->GetOwner())
    {
        if (const auto* OwningController = ::Cast<APlayerController>(Outer))
        { return OwningController->PlayerState; }

        if (auto* OwningPlayerState = ::Cast<APlayerState>(Outer))
        { return OwningPlayerState; }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
