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

    // A SyncPixels request was accepted and waits for capture to start
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_PixelCapturePending);

    // A capture → diff → compress pass is running (readback or background job)
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_PixelSyncInFlight);

    // Client lost instructions to ring wrap (gap in the replicated batch seqs) — needs a pixel
    // baseline before further instructions are trustworthy. Turned into a Server_RequestFullSync
    // on the relay by FProcessor_RenderTarget_ClientNetMaintenance.
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_NeedsBaseline);

    // A baseline request is already on the wire — don't re-send every tick. Cleared (with
    // NeedsBaseline) when a FullSync payload applies.
    CK_DEFINE_ECS_TAG(FTag_RenderTarget_BaselineRequested);

    // Per-feature "done" marker for the snapshot restore-replication pass: once
    // FProcessor_RenderTarget_ReplicateOnRestore has re-derived the target + re-published the
    // restored instruction ring, it stamps this so the (idempotent) re-drive does not repeat.
    // Pairs with ck::FTag_Snapshot_JustRestored (shared, removed by the same pass).
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_RenderTarget_RestoreReplicated);

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
        friend class FProcessor_RenderTarget_ReplicateOnRestore;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        // The drawable target. Managed mode: created by Setup, owned here. Provided mode: the
        // caller's object, pinned so it can't be GC'd out from under the feature.
        TStrongObjectPtr<UTextureRenderTarget2D> _Target;

        // Sequence assigned to the next applied instruction batch. Monotonic per sync entity;
        // doubles as the replicated batch seq on replicating targets.
        int32 _NextBatchSeq = 1;

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

    // Author-side pixel pipeline state. _LastSyncedSnapshot is the CPU baseline the next delta
    // diffs against; the shared ptrs hold the async readback / background-job state polled by
    // FProcessor_RenderTarget_PixelSyncPump.
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
        friend class FProcessor_RenderTarget_ReplicateOnRestore;
        friend class UCk_Utils_RenderTarget_UE;

    private:
        TArray<uint8> _LastSyncedSnapshot;
        FIntPoint _SnapshotSize = FIntPoint::ZeroValue;
        int32 _NextPayloadSeq = 1;

        TSharedPtr<FCk_RenderTarget_Readback> _Readback;
        TSharedPtr<FCk_RenderTarget_PixelJobResult, ESPMode::ThreadSafe> _JobResult;

        // Instruction seq baked into the pixels being captured — carried on the payload so
        // receivers can drop instruction batches the baseline already contains
        int32 _PendingInstructionWatermark = 0;

        // The watermark recorded when _LastSyncedSnapshot was captured — on-demand FullSync
        // baselines built from the snapshot carry this value
        int32 _SnapshotInstructionWatermark = 0;

        // Drives the Interval pixel-sync policy (seeded by Setup; ticked by IntervalSync).
        // FCk_Chrono rather than a CkTimer entity: no delegate UObject is needed for a
        // processor-internal countdown
        FCk_Chrono _IntervalChrono;

    public:
        CK_PROPERTY_GET(_LastSyncedSnapshot);
        CK_PROPERTY_GET(_SnapshotSize);
        CK_PROPERTY_GET(_NextPayloadSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The latest produced (not yet dispatched) compressed pixel payload. DispatchPixelPayload
    // consumes this into per-client chunk streams; each newer payload replaces an unconsumed one.
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

        // Set when the payload re-broadcasts a client upload — the uploader already has these
        // pixels (it authored them); dispatch skips it and promotes its baseline instead
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

    // Receive-side watermark on the sync child: the last instruction batch seq this world has
    // applied, plus the instruction watermark of the last pixel baseline (batches at or below it
    // are already baked into the pixels and must be dropped).
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

    // Replicated instruction batches awaiting apply on this world (client-side). Enqueued by the
    // rep handler, drained by FProcessor_RenderTarget_ApplyReplicatedBatches.
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

    // Stash for batches that arrive before the sync child's Setup has run (CkStateMachine's
    // stash-and-flush precedence, spec §5.4): route here while NeedsSetup is present OR the stash
    // is non-empty (preserves arrival order under back-to-back deliveries), then
    // FlushPendingReplication drains in order.
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

    // One chunk of a compressed pixel payload — the unit both the host's per-player send queues
    // and the client's reassembly inbox carry. Self-contained (payload meta on every chunk) so
    // queues need no side tables.
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

    // Host-side per-player stream: baseline status + the pending chunk queue the pacing
    // processor drains within the per-tick byte budget.
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

    // Host-side stream state on the sync entity. _BaselineJob is the on-demand FullSync compress
    // of _LastSyncedSnapshot kicked when a baseline-less player needs a stream but the produced
    // payload was a delta.
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

    // A client→server stream request (ack or baseline request), enqueued by the relay RPC
    // handlers and drained by the pacing processor.
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

    // Client-side: chunks received for this sync entity, awaiting reassembly (relay RPC enqueues,
    // FProcessor_RenderTarget_ReceivePixels drains).
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

    // Client-side authoritative CPU mirror of the target's pixels plus the in-flight
    // reassembly/apply state. _Pixels is what headless tests hash; _UploadTexture is the
    // transient GPU staging used to draw the mirror into the local render target.
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

        // Authoring client: chunks of a locally-produced pixel payload awaiting their
        // Server_PushPixelChunk sends (paced by ClientNetMaintenance)
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

    // Client-side: a FullSync apply that still needs its Server_AckFullSync sent (retried each
    // tick until the local player's relay channel resolves — without the ack the server never
    // promotes the baseline).
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

    // Authoring client: predicted (locally-applied) draw batches awaiting their relay push.
    // FProcessor_RenderTarget_PushClientBatches flushes via Server_PushDrawBatch, retrying until
    // the local player's channel resolves (the fragment is only removed on a successful push).
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

    // Host: client-pushed draw batches awaiting the authoring gate + apply + republish
    // (FProcessor_RenderTarget_ApplyClientBatches). Sender is stamped server-side by the relay.
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

    // Host: upload reassembly + apply state. One upload applies at a time; chunks of other
    // senders queue behind it (per-sender prefix collection mirrors the client receive path).
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
