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
    // Shared by the pixel-apply processors and the Utils test seam Debug_RedrawTargetFromLastSnapshot.
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
    // Runs on every machine — each world owns its local copy of the target.
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

    // The single local apply site on every machine: one frame's requests, one batch, one canvas pass.
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
        // Shared by the local request path and the replicated replay path. No-ops gracefully when
        // the process cannot render (-nullrhi, dedicated server) or the target failed setup.
        static auto
        DoApplyBatch(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            const TArray<FCk_RenderTarget_DrawCmd>& InCmds) -> void;

        // Returns false (NotReady, retry) until the restored child's Setup has composed Current;
        // true after exactly one repaint. Contract and call site: CkRenderTarget/Claude.md.
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

    // Body-gated to the reconcile authority, deliberately narrower than PixelCapture's authoring
    // gate — CkRenderTarget/Claude.md.
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

    // Skipped while a pass is in flight — the pending tag persists and is consumed on the next
    // pass. Hosts always capture; clients only when _ClientAuthoring is Allowed (body gate).
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

    // GPU completion is inherently poll-based — the accepted exception to event-driven processors.
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

    // Releases stashed batches into the replay queue in arrival order.
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

    // The client-side commit point for replicated instruction batches (echo suppression and
    // ring-wrap gap detection live here, not in the rep handler — CkRenderTarget/Claude.md).
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

    // Host: a player without a baseline cannot apply a delta, so they are flagged for an on-demand
    // FullSync; the local (listen-server) player is skipped — it applied the pixels at capture time.
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

    // Host: sends queued chunks within the per-stream, per-tick byte budget.
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

    // Client: routes chunks that raced the sync child's composition into its inbox once it exists.
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

    // Client: chunks → decompress/patch job → CPU staging mirror → full-rect local target upload.
    // Batches at or below an applied baseline's watermark are already in those pixels and get dropped.
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

    // Client: retries outstanding client→server stream messages (FullSync acks, baseline requests,
    // paced uploads) until the local player's relay channel resolves.
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

    // Authoring client: flushes predicted draw batches to the server. Deliberately has no
    // MarkedDirtyBy — a dirty gate would strand a deferred batch (CkRenderTarget/Claude.md).
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

    // Host: commits client-pushed draw batches under server-side seq assignment, republished with
    // the sender stamped for echo suppression.
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

    // Host: a validated client upload becomes the new authoritative snapshot and is re-published
    // to every OTHER client.
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
