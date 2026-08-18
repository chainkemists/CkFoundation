#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"

#include "CkCore/Chrono/CkChrono.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/Pixels/CkRenderTarget_Readback.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment_Data.h"

#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RenderTarget_UE;
class UTextureRenderTarget2D;
class UTexture2D;
class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_NeedsSetup);

    // Present while the head of the target's draw-request queue awaits its preload batch (drain
    // stalled to preserve per-target draw order). Observability only — nothing gates on it.
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_PendingAssetLoad);

    CK_DEFINE_ECS_TAG(FTag_RenderTarget_PixelCapturePending);

    // Spans the whole capture → diff → compress pass (readback and background job)
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_PixelSyncInFlight);

    // Gap in the replicated batch seqs — instructions are untrustworthy until a pixel baseline
    // lands. ClientNetMaintenance turns this into a Server_RequestFullSync.
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_NeedsBaseline);

    // A baseline request is already on the wire — don't re-send every tick. Cleared (with
    // NeedsBaseline) when a FullSync payload applies.
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_BaselineRequested);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_RenderTarget_Params = FCk_Fragment_RenderTarget_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRENDERTARGET_API FFragment_RenderTarget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_Current);

    public:
        friend class FProcessor_RenderTarget_Setup;
        friend class FProcessor_RenderTarget_HandleRequests;
        friend class FProcessor_RenderTarget_ApplyClientBatches;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        // The drawable target. Managed mode: created by Setup, owned here. Provided mode: the
        // caller's object, pinned so it can't be GC'd out from under the feature.
        TStrongObjectPtr<UTextureRenderTarget2D> _Target;

        // Monotonic per sync entity; doubles as the replicated batch seq on replicating targets.
        int32 _NextBatchSeq = 1;

        // GC does not trace EnTT fragments and applied DrawCmds outlive their requests — these pins
        // keep them valid. Grows with each distinct asset drawn on this target (released only with
        // the entity); eviction-tied release is an open design question.
        TArray<TStrongObjectPtr<UObject>> _PinnedCmdAssets;

    public:
        CK_PROPERTY_GET(_Target);
        CK_PROPERTY_GET(_NextBatchSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRENDERTARGET_API FFragment_RenderTarget_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_Requests);

    public:
        friend class FProcessor_RenderTarget_HandleRequests;
        friend class UCk_Utils_RenderTarget_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_RenderTarget_DrawLine,
            FCk_Request_RenderTarget_DrawTexture,
            FCk_Request_RenderTarget_DrawMaterial,
            FCk_Request_RenderTarget_DrawText,
            FCk_Request_RenderTarget_DrawBox,
            FCk_Request_RenderTarget_DrawBorder,
            FCk_Request_RenderTarget_DrawTriangles,
            FCk_Request_RenderTarget_DrawPolygon,
            FCk_Request_RenderTarget_Clear,
            FCk_Request_RenderTarget_SyncPixels>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // _LastSyncedSnapshot is the CPU baseline the next delta diffs against; the shared ptrs are
    // polled by FProcessor_RenderTarget_PixelSyncPump.
    struct CKRENDERTARGET_API FFragment_RenderTarget_PixelSync
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_PixelSync);

    public:
        friend class FProcessor_RenderTarget_Setup;
        friend class FProcessor_RenderTarget_IntervalSync;
        friend class FProcessor_RenderTarget_PixelCapture;
        friend class FProcessor_RenderTarget_PixelSyncPump;
        friend class FProcessor_RenderTarget_PaceStreams;
        friend class FProcessor_RenderTarget_ReceiveClientUploads;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        TArray<uint8> _LastSyncedSnapshot;
        FIntPoint _SnapshotSize = FIntPoint::ZeroValue;
        int32 _NextPayloadSeq = 1;

        TSharedPtr<FCk_RenderTarget_Readback> _Readback;
        TSharedPtr<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe> _JobResult;

        // Instruction seq baked into the pixels being captured — receivers drop batches at or below it
        int32 _PendingInstructionWatermark = 0;

        // On-demand FullSync baselines built from _LastSyncedSnapshot carry this value
        int32 _SnapshotInstructionWatermark = 0;

        // FCk_Chrono rather than a CkTimer entity: a processor-internal countdown needs no delegate UObject
        FCk_Chrono _IntervalChrono;

    public:
        CK_PROPERTY_GET(_LastSyncedSnapshot);
        CK_PROPERTY_GET(_SnapshotSize);
        CK_PROPERTY_GET(_NextPayloadSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The latest produced (not yet dispatched) compressed pixel payload — a newer one replaces it.
    struct CKRENDERTARGET_API FFragment_RenderTarget_PendingPixelPayload
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_PendingPixelPayload);

    public:
        friend class FProcessor_RenderTarget_PixelSyncPump;
        friend class FProcessor_RenderTarget_ReceiveClientUploads;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        ECk_RenderTarget_PixelPayloadKind _Kind = ECk_RenderTarget_PixelPayloadKind::FullSync;
        int32 _PayloadSeq = 0;
        FIntPoint _Size = FIntPoint::ZeroValue;
        int32 _UncompressedSize = 0;
        TArray<uint8> _Bytes;
        int32 _InstructionWatermark = 0;

        // Set when the payload re-broadcasts a client upload — dispatch skips that player (it
        // authored these pixels) and promotes its baseline instead
        TWeakObjectPtr<APlayerState> _ExcludePlayer;

    public:
        CK_PROPERTY_GET(_Kind);
        CK_PROPERTY_GET(_PayloadSeq);
        CK_PROPERTY_GET(_Size);
        CK_PROPERTY_GET(_UncompressedSize);
        CK_PROPERTY_GET(_Bytes);
        CK_PROPERTY_GET(_InstructionWatermark);
        CK_PROPERTY_GET(_ExcludePlayer);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Receive-side watermarks on the sync child. Batches at or below _BaselineInstructionWatermark
    // are already baked into the last pixel baseline and must be dropped.
    struct CKRENDERTARGET_API FFragment_RenderTarget_ClientReplayState
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_ClientReplayState);

    public:
        friend class FProcessor_RenderTarget_ApplyReplicatedBatches;
        friend class FProcessor_RenderTarget_ReceivePixels;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        int32 _LastAppliedSeq = 0;
        int32 _BaselineInstructionWatermark = 0;

    public:
        CK_PROPERTY_GET(_LastAppliedSeq);
        CK_PROPERTY_GET(_BaselineInstructionWatermark);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side: replicated batches enqueued by the rep handler, drained by ApplyReplicatedBatches.
    struct CKRENDERTARGET_API FFragment_RenderTarget_ReplayQueue
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_ReplayQueue);

    public:
        friend class FProcessor_RenderTarget_ApplyReplicatedBatches;
        friend class FProcessor_RenderTarget_FlushPendingReplication;

    private:
        TArray<FCk_RenderTarget_InstructionBatch> _Queue;

    public:
        CK_PROPERTY(_Queue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Stash for batches that arrive before the sync child's Setup has run. Routing rules and why
    // they are shaped that way: CkRenderTarget/Claude.md.
    struct CKRENDERTARGET_API FFragment_RenderTarget_PendingReplicationBatches
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_PendingReplicationBatches);

    public:
        friend class FProcessor_RenderTarget_FlushPendingReplication;

    private:
        TArray<FCk_RenderTarget_InstructionBatch> _Stash;

    public:
        CK_PROPERTY(_Stash);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The saved channel a load handed this sync child, parked until Setup has resolved the drawable target the
    // replay needs. The persistence handler may not wait for Setup itself — a load holds a restored entity out of
    // every non-kernel processor's view until its payloads have applied, so a handler that waits for Setup waits
    // for something that cannot happen and its payload is dropped at the apply timeout. Same shape, and the same
    // reason, as FFragment_Sm_HydrationResume.
    struct CKRENDERTARGET_API FFragment_RenderTarget_HydrationReplay
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_HydrationReplay);

    public:
        friend class FProcessor_RenderTarget_HydrationReplay;

    private:
        FCk_RenderTarget_ChannelState _Channel;

    public:
        // Seeded by the save-transport Apply handler, which is not a friend.
        auto Populate(const FCk_RenderTarget_ChannelState& InChannel) -> void { _Channel = InChannel; }

    public:
        CK_PROPERTY_GET(_Channel);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The unit both the host's per-player send queues and the client's reassembly inbox carry.
    // Payload meta rides on every chunk so those queues need no side tables.
    struct CKRENDERTARGET_API FCk_RenderTarget_PixelChunk
    {
    public:
        CK_GENERATED_BODY(FCk_RenderTarget_PixelChunk);

    private:
        ECk_RenderTarget_PixelPayloadKind _Kind = ECk_RenderTarget_PixelPayloadKind::FullSync;
        int32 _PayloadSeq = 0;
        int32 _ChunkIdx = 0;
        int32 _NumChunks = 0;
        int32 _UncompressedSize = 0;
        FIntPoint _Size = FIntPoint::ZeroValue;
        int32 _InstructionWatermark = 0;
        TArray<uint8> _Bytes;

    public:
        CK_PROPERTY(_Kind);
        CK_PROPERTY(_PayloadSeq);
        CK_PROPERTY(_ChunkIdx);
        CK_PROPERTY(_NumChunks);
        CK_PROPERTY(_UncompressedSize);
        CK_PROPERTY(_Size);
        CK_PROPERTY(_InstructionWatermark);
        CK_PROPERTY(_Bytes);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host-side per-player stream; PaceStreams drains _Chunks within the per-tick byte budget.
    struct CKRENDERTARGET_API FCk_RenderTarget_PlayerStream
    {
    public:
        CK_GENERATED_BODY(FCk_RenderTarget_PlayerStream);

    public:
        bool _HasBaseline = false;
        bool _NeedsFullSync = false;
        int32 _LastSentPayloadSeq = 0;
        TArray<FCk_RenderTarget_PixelChunk> _Chunks;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host-side stream state on the sync entity. _BaselineJob is the on-demand FullSync compress of
    // _LastSyncedSnapshot, kicked when a baseline-less player needs a stream but the payload was a delta.
    struct CKRENDERTARGET_API FFragment_RenderTarget_HostStreams
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_HostStreams);

    public:
        friend class FProcessor_RenderTarget_DispatchPixelPayload;
        friend class FProcessor_RenderTarget_PaceStreams;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        TMap<TWeakObjectPtr<APlayerState>, FCk_RenderTarget_PlayerStream> _Streams;
        TSharedPtr<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe> _BaselineJob;
        int32 _BaselineJobInstructionWatermark = 0;

    public:
        CK_PROPERTY_GET(_Streams);
    };

    // --------------------------------------------------------------------------------------------------------------------

    enum class ECk_RenderTarget_StreamRequestKind : uint8
    {
        AckFullSync,
        RequestFullSync
    };

    // Enqueued by the relay RPC handlers, drained by PaceStreams.
    struct CKRENDERTARGET_API FCk_RenderTarget_StreamRequest
    {
    public:
        CK_GENERATED_BODY(FCk_RenderTarget_StreamRequest);

    private:
        ECk_RenderTarget_StreamRequestKind _Kind = ECk_RenderTarget_StreamRequestKind::AckFullSync;
        TWeakObjectPtr<APlayerState> _Player;
        int32 _PayloadSeq = 0;

    public:
        CK_PROPERTY_GET(_Kind);
        CK_PROPERTY_GET(_Player);
        CK_PROPERTY_GET(_PayloadSeq);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_RenderTarget_StreamRequest, _Kind, _Player, _PayloadSeq);
    };

    struct CKRENDERTARGET_API FFragment_RenderTarget_HostInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_HostInbox);

    public:
        friend class FProcessor_RenderTarget_PaceStreams;

    private:
        TArray<FCk_RenderTarget_StreamRequest> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side: relay RPC enqueues, FProcessor_RenderTarget_ReceivePixels drains.
    struct CKRENDERTARGET_API FFragment_RenderTarget_ChunkInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_ChunkInbox);

    public:
        friend class FProcessor_RenderTarget_ReceivePixels;

    private:
        TArray<FCk_RenderTarget_PixelChunk> _Chunks;

    public:
        CK_PROPERTY(_Chunks);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side, on the OWNER entity: chunks that arrived before the addressed sync child
    // composed. FProcessor_RenderTarget_FlushOwnerChunkStash routes them once it exists.
    struct CKRENDERTARGET_API FCk_RenderTarget_StashedChunk
    {
    public:
        CK_GENERATED_BODY(FCk_RenderTarget_StashedChunk);

    private:
        FGameplayTag _SyncName;
        FCk_RenderTarget_PixelChunk _Chunk;

    public:
        CK_PROPERTY_GET(_SyncName);
        CK_PROPERTY_GET(_Chunk);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_RenderTarget_StashedChunk, _SyncName, _Chunk);
    };

    struct CKRENDERTARGET_API FFragment_RenderTarget_OwnerChunkStash
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_OwnerChunkStash);

    public:
        friend class FProcessor_RenderTarget_FlushOwnerChunkStash;

    private:
        TArray<FCk_RenderTarget_StashedChunk> _StashedChunks;

    public:
        CK_PROPERTY(_StashedChunks);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side authoritative CPU mirror of the target's pixels: _Pixels is what headless tests
    // hash; _UploadTexture is the transient GPU staging.
    struct CKRENDERTARGET_API FFragment_RenderTarget_ClientStaging
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_ClientStaging);

    public:
        friend class FProcessor_RenderTarget_ReceivePixels;
        friend class FProcessor_RenderTarget_ClientNetMaintenance;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        TArray<uint8> _Pixels;
        FIntPoint _Size = FIntPoint::ZeroValue;

        TArray<FCk_RenderTarget_PixelChunk> _Assembling;

        // Authoring client: locally-produced payload awaiting its paced Server_PushPixelChunk sends
        TArray<FCk_RenderTarget_PixelChunk> _UploadChunks;

        TSharedPtr<FCk_RenderTarget_PixelApplyJobResult, ESPMode::ThreadSafe> _ApplyJob;
        ECk_RenderTarget_PixelPayloadKind _ApplyJobKind = ECk_RenderTarget_PixelPayloadKind::FullSync;
        int32 _ApplyJobPayloadSeq = 0;
        int32 _ApplyJobInstructionWatermark = 0;

        ECk_RenderTarget_PixelPayloadKind _LastAppliedPayloadKind = ECk_RenderTarget_PixelPayloadKind::FullSync;
        int32 _LastAppliedPayloadSeq = 0;

        TStrongObjectPtr<UTexture2D> _UploadTexture;

    public:
        CK_PROPERTY_GET(_Pixels);
        CK_PROPERTY_GET(_Size);
        CK_PROPERTY_GET(_LastAppliedPayloadKind);
        CK_PROPERTY_GET(_LastAppliedPayloadSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side: a FullSync apply whose Server_AckFullSync is retried each tick until the local
    // player's relay channel resolves — without the ack the server never promotes the baseline.
    struct CKRENDERTARGET_API FFragment_RenderTarget_PendingAck
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_PendingAck);

    public:
        friend class FProcessor_RenderTarget_ReceivePixels;
        friend class FProcessor_RenderTarget_ClientNetMaintenance;

    private:
        int32 _PayloadSeq = 0;

    public:
        CK_PROPERTY(_PayloadSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Authoring client: predicted (locally-applied) draw batches awaiting their relay push. Removed
    // only on a successful push — that is what terminates the retry.
    struct CKRENDERTARGET_API FFragment_RenderTarget_PendingClientBatches
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_PendingClientBatches);

    public:
        friend class FProcessor_RenderTarget_HandleRequests;
        friend class FProcessor_RenderTarget_PushClientBatches;

    private:
        TArray<FCk_RenderTarget_InstructionBatch> _Batches;

    public:
        CK_PROPERTY(_Batches);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: client-pushed batches awaiting ApplyClientBatches. Sender is stamped server-side by the relay.
    struct CKRENDERTARGET_API FFragment_RenderTarget_ServerIngressBatches
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_ServerIngressBatches);

    public:
        friend class FProcessor_RenderTarget_ApplyClientBatches;

    private:
        TArray<FCk_RenderTarget_InstructionBatch> _Batches;

    public:
        CK_PROPERTY(_Batches);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: one chunk of a client pixel upload, tagged with its (server-resolved) sender.
    struct CKRENDERTARGET_API FCk_RenderTarget_UploadChunk
    {
    public:
        CK_GENERATED_BODY(FCk_RenderTarget_UploadChunk);

    private:
        TWeakObjectPtr<APlayerState> _Sender;
        FCk_RenderTarget_PixelChunk _Chunk;

    public:
        CK_PROPERTY_GET(_Sender);
        CK_PROPERTY_GET(_Chunk);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_RenderTarget_UploadChunk, _Sender, _Chunk);
    };

    // Host: upload reassembly + apply state. One upload applies at a time; chunks of other senders
    // queue behind it.
    struct CKRENDERTARGET_API FFragment_RenderTarget_UploadAssembly
    {
    public:
        CK_GENERATED_BODY(FFragment_RenderTarget_UploadAssembly);

    public:
        friend class FProcessor_RenderTarget_ReceiveClientUploads;

    public:
        // Per-sender sliding-window budget for ClientUploadMaxBytesPerSecond enforcement
        struct FUploadBudget
        {
            float _WindowStartSeconds = 0.0f;
            int32 _BytesThisWindow = 0;
        };

    private:
        TArray<FCk_RenderTarget_UploadChunk> _Inbox;

        TSharedPtr<FCk_RenderTarget_PixelApplyJobResult, ESPMode::ThreadSafe> _ApplyJob;
        TWeakObjectPtr<APlayerState> _ApplyJobSender;

        TMap<TWeakObjectPtr<APlayerState>, FUploadBudget> _Budgets;

        // GPU staging for redrawing accepted uploads into the host's local target
        TStrongObjectPtr<UTexture2D> _UploadTexture;

    public:
        CK_PROPERTY(_Inbox);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfRenderTargets, FCk_Handle_RenderTarget);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKRENDERTARGET_API,
        RenderTarget_OnInstructionsApplied,
        FCk_Delegate_RenderTarget_OnInstructionsApplied,
        FCk_Handle_RenderTarget,
        int32,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKRENDERTARGET_API,
        RenderTarget_OnPixelPayloadProduced,
        FCk_Delegate_RenderTarget_OnPixelPayloadProduced,
        FCk_Handle_RenderTarget,
        ECk_RenderTarget_PixelPayloadKind,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKRENDERTARGET_API,
        RenderTarget_OnPixelPayloadApplied,
        FCk_Delegate_RenderTarget_OnPixelPayloadApplied,
        FCk_Handle_RenderTarget,
        ECk_RenderTarget_PixelPayloadKind,
        int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKRENDERTARGET_API,
        RenderTarget_OnClientBaselineEstablished,
        FCk_Delegate_RenderTarget_OnClientBaselineEstablished,
        FCk_Handle_RenderTarget,
        TWeakObjectPtr<APlayerState>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKRENDERTARGET_API,
        RenderTarget_OnClientUploadApplied,
        FCk_Delegate_RenderTarget_OnClientUploadApplied,
        FCk_Handle_RenderTarget,
        TWeakObjectPtr<APlayerState>);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_RenderTarget_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
