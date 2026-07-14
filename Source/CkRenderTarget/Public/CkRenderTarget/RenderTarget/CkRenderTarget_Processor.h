#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvas;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck_render_target_processor
{
    // Draws a CPU pixel buffer into the entity's local render target through a transient
    // upload texture. Shared by the pixel-apply processors and the Utils test seam
    // (Debug_RedrawTargetFromLastSnapshot). Defined in CkRenderTarget_Processor.cpp.
    CKRENDERTARGET_API auto
    DrawPixelsToTarget(
        const FCk_Handle_RenderTarget& InRenderTargetEntity,
        const ck::FFragment_RenderTarget_Current& InCurrent,
        const TArray<uint8>& InPixels,
        const FIntPoint& InSize,
        TStrongObjectPtr<UTexture2D>& InOutUploadTexture) -> void;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Resolves the drawable target for the sync entity: Managed mode creates a transient RGBA8
    // render target from params, Provided mode validates and pins the caller's object. Runs on
    // every machine — each world owns its local copy of the target.
    class CKRENDERTARGET_API FProcessor_RenderTarget_Setup : public ck_exp::TProcessor<
        FProcessor_RenderTarget_Setup,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_Current>,
        FTag_RenderTarget_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FTag_RenderTarget_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains the frame's draw requests, normalizes them into one FCk_RenderTarget_DrawCmd batch,
    // applies the batch to the local render target through a single canvas pass, and broadcasts
    // OnInstructionsApplied with the batch seq. Replication of the batch (channel A) attaches in
    // a later phase — this processor is the single apply site on every machine.
    class CKRENDERTARGET_API FProcessor_RenderTarget_HandleRequests : public ck_exp::TProcessor<
        FProcessor_RenderTarget_HandleRequests,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_Requests>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_Setup>;
        using MarkedDirtyBy = FFragment_RenderTarget_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawLine& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawTexture& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawMaterial& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawText& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawBox& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawBorder& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawTriangles& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawPolygon& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_Clear& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_SyncPixels& InRequest) -> void;

    public:
        static auto
        DoPublishBatch(
            HandleType InRenderTargetEntity,
            int32 InBatchSeq,
            const TArray<FCk_RenderTarget_DrawCmd>& InCmds,
            APlayerState* InSender = nullptr) -> void;

    public:
        // Applies a normalized batch to the entity's local render target. Shared by the local
        // request path and the replicated replay path. No-ops gracefully when
        // the process cannot render (-nullrhi, dedicated server) or the target failed setup.
        static auto
        DoApplyBatch(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            const TArray<FCk_RenderTarget_DrawCmd>& InCmds) -> void;

        // Save-load hydration (Phase 4B): re-drives a v3-restored sync CHILD from its saved single-channel
        // payload — refills the persisted ring, restores the author seq watermark, repaints, and (Replicates)
        // re-publishes into a fresh owner container. Lives here because this class is already a friend of
        // FFragment_RenderTarget_Current (for the _NextBatchSeq write) and owns DoApplyBatch (the repaint
        // primitive). Returns false (NotReady, retry) until the child's Setup has composed Current; true
        // (Applied) after exactly one repaint. Called from the hydration-scope branch of the
        // FCk_RepData_RenderTarget Apply handler (CkRenderTarget_Replication.cpp).
        static auto
        HydrateFromSavedChannel(
            FCk_Handle& InChild,
            const FCk_RenderTarget_ChannelState& InChannel) -> bool;

    private:
        static auto
        DoApplyCmdToCanvas(
            UCanvas& InCanvas,
            const FCk_RenderTarget_DrawCmd& InCmd) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drives the Interval pixel-sync policy: ticks the per-entity chrono and requests a capture
    // each time it elapses. Body-gated to capture authors (same gate as PixelCapture). GPU
    // completion of the resulting pass is handled by the normal capture pipeline.
    class CKRENDERTARGET_API FProcessor_RenderTarget_IntervalSync : public ck_exp::TProcessor<
        FProcessor_RenderTarget_IntervalSync,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_PixelSync>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunBefore = TDepList<FProcessor_RenderTarget_PixelCapture>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_PixelSync& InPixelSync)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Starts a GPU readback for sync entities with an accepted SyncPixels request. Skipped while
    // a pass is already in flight (the pending tag persists and is consumed on the next pass).
    // Authoring-gated in the body: hosts always capture, clients only when _ClientAuthoring is
    // Allowed (the pixel-upload path).
    class CKRENDERTARGET_API FProcessor_RenderTarget_PixelCapture : public ck_exp::TProcessor<
        FProcessor_RenderTarget_PixelCapture,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadOnly<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_PixelSync>,
        FTag_RenderTarget_PixelCapturePending,
        TExclude<FTag_RenderTarget_PixelSyncInFlight>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_HandleRequests>;
        using MarkedDirtyBy = FTag_RenderTarget_PixelCapturePending;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drives the in-flight pixel pass to completion: polls the readback fence/copy, hands the
    // captured pixels to the diff+compress background job, then lands the produced payload
    // (FFragment_RenderTarget_PendingPixelPayload + OnPixelPayloadProduced) or drops it on
    // zero-diff. GPU completion is inherently poll-based — the framework-accepted exception to
    // event-driven.
    class CKRENDERTARGET_API FProcessor_RenderTarget_PixelSyncPump : public ck_exp::TProcessor<
        FProcessor_RenderTarget_PixelSyncPump,
        FCk_Handle_RenderTarget,
        ck::TReadWrite<FFragment_RenderTarget_PixelSync>,
        FTag_RenderTarget_PixelSyncInFlight,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_PixelCapture>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PixelSync& InPixelSync)
            -> void;

    private:
        static auto
        DoFinishPass(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PixelSync& InPixelSync) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Releases batches stashed by the rep handler (arrivals before client-side Setup completed)
    // into the replay queue in arrival order. Runs before ApplyReplicatedBatches so released
    // batches commit in the same tick (SM FlushPendingReplication_Drain pattern).
    class CKRENDERTARGET_API FProcessor_RenderTarget_FlushPendingReplication : public ck_exp::TProcessor<
        FProcessor_RenderTarget_FlushPendingReplication,
        FCk_Handle_RenderTarget,
        ck::TReadWrite<FFragment_RenderTarget_PendingReplicationBatches>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_Setup>;
        using MarkedDirtyBy = FFragment_RenderTarget_PendingReplicationBatches;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PendingReplicationBatches& InStash)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side commit point for replicated instruction batches: drains the replay queue in
    // arrival order, drops batches at or below the watermark / pixel-baseline watermark, detects
    // ring-wrap gaps (FTag_RenderTarget_NeedsBaseline), applies via the shared DoApplyBatch, and
    // broadcasts OnInstructionsApplied with the wire seq.
    class CKRENDERTARGET_API FProcessor_RenderTarget_ApplyReplicatedBatches : public ck_exp::TProcessor<
        FProcessor_RenderTarget_ApplyReplicatedBatches,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_ReplayQueue>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_FlushPendingReplication>;
        using MarkedDirtyBy = FFragment_RenderTarget_ReplayQueue;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ReplayQueue& InReplayQueue)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: consumes the produced pixel payload into per-player chunk queues. Players without a
    // baseline cannot apply a delta — they're flagged for an on-demand FullSync instead (built by
    // PaceStreams from the snapshot). The local (listen-server) player is skipped — the host
    // already applied the pixels at capture time.
    class CKRENDERTARGET_API FProcessor_RenderTarget_DispatchPixelPayload : public ck_exp::TProcessor<
        FProcessor_RenderTarget_DispatchPixelPayload,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_PendingPixelPayload>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_PixelSyncPump>;
        using MarkedDirtyBy = FFragment_RenderTarget_PendingPixelPayload;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_PendingPixelPayload& InPayload)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: drives the per-player streams every tick — drains the client request inbox (acks /
    // baseline requests), manages the on-demand FullSync baseline job, and sends queued chunks
    // within the per-stream byte budget via reliable Client RPCs on each player's relay channel.
    class CKRENDERTARGET_API FProcessor_RenderTarget_PaceStreams : public ck_exp::TProcessor<
        FProcessor_RenderTarget_PaceStreams,
        FCk_Handle_RenderTarget,
        ck::TReadWrite<FFragment_RenderTarget_HostStreams>,
        ck::TReadWrite<FFragment_RenderTarget_PixelSync>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_DispatchPixelPayload>;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams,
            FFragment_RenderTarget_PixelSync& InPixelSync)
            -> void;

    private:
        static auto
        DoDrainInbox(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams) -> void;

        static auto
        DoManageBaselineJob(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams,
            FFragment_RenderTarget_PixelSync& InPixelSync) -> void;

        static auto
        DoSendChunks(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client: routes chunks stashed on the owner (arrivals that raced the sync child's
    // composition) into the child's inbox once it exists.
    class CKRENDERTARGET_API FProcessor_RenderTarget_FlushOwnerChunkStash : public ck_exp::TProcessor<
        FProcessor_RenderTarget_FlushOwnerChunkStash,
        FCk_Handle,
        ck::TReadWrite<FFragment_RenderTarget_OwnerChunkStash>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOwnerEntity,
            FFragment_RenderTarget_OwnerChunkStash& InStash)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client: reassembles inbound chunks, runs the background decompress/patch job, lands the
    // result in the CPU staging mirror, uploads to the local target (full-rect, v1), queues the
    // FullSync ack, and reconciles the instruction watermark (batches at or below the baseline's
    // watermark are dropped — they're baked into the pixels).
    class CKRENDERTARGET_API FProcessor_RenderTarget_ReceivePixels : public ck_exp::TProcessor<
        FProcessor_RenderTarget_ReceivePixels,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_ClientStaging>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_FlushOwnerChunkStash>;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging)
            -> void;

    private:
        static auto
        DoFinishApply(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging) -> void;

        static auto
        DoUploadStagingToTarget(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Client: retries outstanding client→server stream messages each tick until the local
    // player's relay channel resolves — FullSync acks (FFragment_RenderTarget_PendingAck) and
    // baseline requests (FTag_RenderTarget_NeedsBaseline → Server_RequestFullSync).
    class CKRENDERTARGET_API FProcessor_RenderTarget_ClientNetMaintenance : public ck_exp::TProcessor<
        FProcessor_RenderTarget_ClientNetMaintenance,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_ClientStaging>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_ReceivePixels>;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_ClientStaging& InStaging)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Authoring client: flushes predicted draw batches to the server via Server_PushDrawBatch.
    // Runs every tick while a pending batch exists — the relay channel may not have resolved yet
    // and a MarkedDirtyBy gate would strand a deferred batch until the next draw (the SM
    // PushOwningClientBatch lesson).
    class CKRENDERTARGET_API FProcessor_RenderTarget_PushClientBatches : public ck_exp::TProcessor<
        FProcessor_RenderTarget_PushClientBatches,
        FCk_Handle_RenderTarget,
        ck::TReadWrite<FFragment_RenderTarget_PendingClientBatches>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_HandleRequests>;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PendingClientBatches& InPending)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: commits client-pushed draw batches — the _ClientAuthoring gate, the local apply, the
    // server-side seq assignment, and the republish (with sender stamped for echo suppression).
    class CKRENDERTARGET_API FProcessor_RenderTarget_ApplyClientBatches : public ck_exp::TProcessor<
        FProcessor_RenderTarget_ApplyClientBatches,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadWrite<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_ServerIngressBatches>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_RenderTarget_HandleRequests>;
        using MarkedDirtyBy = FFragment_RenderTarget_ServerIngressBatches;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ServerIngressBatches& InIngress)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Host: reassembles client pixel uploads, validates them (authoring gate + size caps),
    // applies the result as the new authoritative snapshot (+ local target redraw), bumps the
    // channel's _PixelEpoch, and re-publishes the payload to every OTHER client.
    class CKRENDERTARGET_API FProcessor_RenderTarget_ReceiveClientUploads : public ck_exp::TProcessor<
        FProcessor_RenderTarget_ReceiveClientUploads,
        FCk_Handle_RenderTarget,
        ck::TReadOnly<FFragment_RenderTarget_Params>,
        ck::TReadOnly<FFragment_RenderTarget_Current>,
        ck::TReadWrite<FFragment_RenderTarget_PixelSync>,
        ck::TReadWrite<FFragment_RenderTarget_UploadAssembly>,
        TExclude<FTag_RenderTarget_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        // After the capture pipeline (both write PixelSync), before dispatch (which consumes the
        // payload this processor republishes).
        using RunAfter = TDepList<FProcessor_RenderTarget_PixelCapture, FProcessor_RenderTarget_PixelSyncPump>;
        using RunBefore = TDepList<FProcessor_RenderTarget_DispatchPixelPayload>;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync,
            FFragment_RenderTarget_UploadAssembly& InAssembly)
            -> void;

    private:
        static auto
        DoFinishUploadApply(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync,
            FFragment_RenderTarget_UploadAssembly& InAssembly) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
