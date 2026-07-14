#include "CkRenderTarget_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkRenderTarget/Net/CkRenderTargetRelay_Actor.h"
#include "CkRenderTarget/Net/CkRenderTargetRelay_Subsystem.h"
#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/Pixels/CkRenderTarget_PixelMath.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Utils.h"
#include "CkRenderTarget/Settings/CkRenderTarget_Settings.h"

#include <Engine/Canvas.h>
#include <Engine/Engine.h>
#include <Engine/Font.h>
#include <Engine/Texture.h>
#include <Engine/Texture2D.h>
#include <Engine/TextureRenderTarget2D.h>
#include <Engine/World.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <Kismet/KismetRenderingLibrary.h>
#include <Materials/MaterialInterface.h>
#include <Misc/App.h>
#include <RHICommandList.h>
#include <RenderingThread.h>
#include <Rendering/Texture2DResource.h>
#include <TextureResource.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_PixelCapture);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_PixelSyncPump);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_FlushPendingReplication);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_ApplyReplicatedBatches);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_DispatchPixelPayload);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_PaceStreams);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_FlushOwnerChunkStash);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_ReceivePixels);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_ClientNetMaintenance);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_PushClientBatches);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_ApplyClientBatches);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_ReceiveClientUploads);
CK_REGISTER_PROCESSOR(ck::FProcessor_RenderTarget_IntervalSync);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkRenderTarget"), STATGROUP_CkRenderTarget, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("RenderTarget Pixel Bytes Sent"), STAT_CkRenderTarget_PixelBytesSent, STATGROUP_CkRenderTarget);
DECLARE_DWORD_COUNTER_STAT(TEXT("RenderTarget Pixel Payloads Produced"), STAT_CkRenderTarget_PayloadsProduced, STATGROUP_CkRenderTarget);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_render_target_processor
{

    // Sync-or-null relay channel resolution for a specific player (mirrors CkStateMachine's
    // Acquire_RelayChannel): callers retry next tick on null — never block, never subscribe.
    auto
    ResolveRelayChannel_ForPlayer(
        UWorld* InWorld,
        APlayerState* InPlayerState) -> ACk_RenderTargetRelay_UE*
    {
        if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}) || ck::Is_NOT_Valid(InPlayerState))
        { return nullptr; }

        auto* Subsystem = InWorld->GetSubsystem<UCk_RenderTargetRelay_Subsystem_UE>();

        if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        auto Pending = Subsystem->Request_AcquireChannel_ForPlayer(InPlayerState);
        const auto Result = Subsystem->Try_ResolvePending(Pending);

        return ::Cast<ACk_RenderTargetRelay_UE>(Result.Get_ChannelActor().Get());
    }

    auto
    Get_LocalPlayerState(
        UWorld* InWorld) -> APlayerState*
    {
        if (ck::Is_NOT_Valid(InWorld, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        const auto* LocalController = InWorld->GetFirstPlayerController();

        if (ck::Is_NOT_Valid(LocalController, ck::IsValid_Policy_NullptrOnly{}))
        { return nullptr; }

        return LocalController->PlayerState;
    }

    // Draws a CPU pixel buffer into the entity's local render target through a transient
    // upload texture (full-rect; per-block dirty rects are a possible later optimization).
    // No-ops when the process cannot render. Shared by the client staging apply and the host's
    // accepted-upload redraw.
    //
    // The final hop is an RHI CopyTexture, NOT a canvas draw: canvas writes go through the
    // material/gamma pipeline and are not byte-preserving (display-gamma encode on write), and
    // the upload texture must match the target's native pixel format (RTF_RGBA8 is PF_B8G8R8A8
    // under the hood) or the channels swizzle. Staging bytes are raw readback bytes — the redraw
    // must put back EXACTLY those bytes or every reconcile visibly corrupts the board
    // (pinned by CkAutoTest_RenderTarget_GpuRoundTrip_BytePreserving).
    auto
    DrawPixelsToTarget(
        const FCk_Handle_RenderTarget& InRenderTargetEntity,
        const ck::FFragment_RenderTarget_Current& InCurrent,
        const TArray<uint8>& InPixels,
        const FIntPoint& InSize,
        TStrongObjectPtr<UTexture2D>& InOutUploadTexture) -> void
    {
        if (NOT FApp::CanEverRender())
        { return; }

        auto* Target = InCurrent.Get_Target().Get();

        if (ck::Is_NOT_Valid(Target))
        { return; }

        if (InSize.X <= 0 || InSize.Y <= 0 || InPixels.Num() != InSize.X * InSize.Y * ck::render_target::pixel::BytesPerPixel)
        { return; }

        if (Target->SizeX != InSize.X || Target->SizeY != InSize.Y)
        {
            ck::render_target::Warning(
                TEXT("RenderTarget [{}] staging size [{}x{}] does not match the target [{}x{}] — redraw skipped"),
                InRenderTargetEntity, InSize.X, InSize.Y, Target->SizeX, Target->SizeY);
            return;
        }

        const auto TargetFormat = Target->GetFormat();
        auto* UploadTexture = InOutUploadTexture.Get();

        if (ck::Is_NOT_Valid(UploadTexture)
            || UploadTexture->GetSizeX() != InSize.X
            || UploadTexture->GetSizeY() != InSize.Y
            || UploadTexture->GetPixelFormat() != TargetFormat)
        {
            UploadTexture = UTexture2D::CreateTransient(InSize.X, InSize.Y, TargetFormat);

            if (ck::Is_NOT_Valid(UploadTexture))
            { return; }

            UploadTexture->NeverStream = true;
            UploadTexture->SRGB = false;
            UploadTexture->UpdateResource();
            InOutUploadTexture = TStrongObjectPtr{UploadTexture};
        }

        // UpdateTextureRegions consumes the data on the render thread — hand it a heap copy
        // with a cleanup fn (the source buffer remains owned by the caller's fragment).
        const auto NumBytes = InPixels.Num();
        auto* DataCopy = static_cast<uint8*>(FMemory::Malloc(NumBytes));
        FMemory::Memcpy(DataCopy, InPixels.GetData(), NumBytes);

        auto* Region = new FUpdateTextureRegion2D{0, 0, 0, 0,
            static_cast<uint32>(InSize.X), static_cast<uint32>(InSize.Y)};

        UploadTexture->UpdateTextureRegions(0, 1, Region,
            InSize.X * ck::render_target::pixel::BytesPerPixel, ck::render_target::pixel::BytesPerPixel, DataCopy,
            [](uint8* InData, const FUpdateTextureRegion2D* InRegions) -> void
            {
                FMemory::Free(InData);
                delete InRegions;
            });

        // Enqueued after the region update above, so the copy sees the new bytes. Both
        // resources are pinned by fragments (TStrongObjectPtr), so they outlive the command.
        ENQUEUE_RENDER_COMMAND(CkRenderTarget_CopyStagingToTarget)(
            [UploadResource = UploadTexture->GetResource(),
             TargetResource = Target->GameThread_GetRenderTargetResource()]
            (FRHICommandListImmediate& RHICmdList) -> void
            {
                if (UploadResource == nullptr || TargetResource == nullptr)
                { return; }

                FRHITexture* SrcRHI = UploadResource->GetTexture2DRHI();
                FRHITexture* DstRHI = TargetResource->GetRenderTargetTexture();

                if (SrcRHI == nullptr || DstRHI == nullptr)
                { return; }

                RHICmdList.Transition(FRHITransitionInfo{SrcRHI, ERHIAccess::Unknown, ERHIAccess::CopySrc});
                RHICmdList.Transition(FRHITransitionInfo{DstRHI, ERHIAccess::Unknown, ERHIAccess::CopyDest});

                RHICmdList.CopyTexture(SrcRHI, DstRHI, FRHICopyTextureInfo{});

                RHICmdList.Transition(FRHITransitionInfo{SrcRHI, ERHIAccess::CopySrc, ERHIAccess::SRVMask});
                RHICmdList.Transition(FRHITransitionInfo{DstRHI, ERHIAccess::CopyDest, ERHIAccess::SRVMask});
            });
    }

    // Resolves (or creates) the drawable RGBA8 target for a sync entity from its params — the
    // managed/provided switch used by FProcessor_RenderTarget_Setup, which runs on both fresh spawns
    // and snapshot loads (v3 rebuild+hydrate re-Constructs the entity, re-running Setup). Returns null
    // after a loud ensure on misconfiguration; the caller pins the result into FFragment_RenderTarget_Current::_Target.
    auto
    ResolveDrawableTarget(
        const FCk_Handle_RenderTarget& InRenderTargetEntity,
        const ck::FFragment_RenderTarget_Params& InParams) -> UTextureRenderTarget2D*
    {
        switch (InParams.Get_TargetMode())
        {
            case ECk_RenderTarget_TargetMode::UseProvided:
            {
                auto* ProvidedTarget = InParams.Get_ProvidedTarget().Get();

                CK_ENSURE_IF_NOT(ck::IsValid(ProvidedTarget),
                    TEXT("RenderTarget [{}] is set to UseProvided but no render target was provided."),
                    InRenderTargetEntity)
                { return nullptr; }

                CK_ENSURE_IF_NOT(ProvidedTarget->RenderTargetFormat == ETextureRenderTargetFormat::RTF_RGBA8,
                    TEXT("RenderTarget [{}] was provided target [{}] with format [{}]. Only RTF_RGBA8 is supported in v1."),
                    InRenderTargetEntity, ProvidedTarget,
                    static_cast<int32>(ProvidedTarget->RenderTargetFormat))
                { return nullptr; }

                return ProvidedTarget;
            }
            case ECk_RenderTarget_TargetMode::CreateManaged:
            {
                const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);

                CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
                    TEXT("RenderTarget [{}] could not resolve a World to create its managed target in."),
                    InRenderTargetEntity)
                { return nullptr; }

                const auto RequestedSize = InParams.Get_Size();

                CK_ENSURE_IF_NOT(RequestedSize.X > 0 && RequestedSize.Y > 0,
                    TEXT("RenderTarget [{}] has invalid managed size [{}x{}]."),
                    InRenderTargetEntity, RequestedSize.X, RequestedSize.Y)
                { return nullptr; }

                const auto ClampedSize = FIntPoint
                {
                    FMath::Min(RequestedSize.X, UCk_Utils_RenderTarget_Settings_UE::Get_MaxManagedSize()),
                    FMath::Min(RequestedSize.Y, UCk_Utils_RenderTarget_Settings_UE::Get_MaxManagedSize())
                };

                if (ClampedSize != RequestedSize)
                {
                    ck::render_target::Warning(
                        TEXT("RenderTarget [{}] managed size [{}x{}] exceeds the cap [{}] — clamped to [{}x{}]"),
                        InRenderTargetEntity, RequestedSize.X, RequestedSize.Y,
                        UCk_Utils_RenderTarget_Settings_UE::Get_MaxManagedSize(), ClampedSize.X, ClampedSize.Y);
                }

                auto* CreatedTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
                    World, ClampedSize.X, ClampedSize.Y, ETextureRenderTargetFormat::RTF_RGBA8);

                CK_ENSURE_IF_NOT(ck::IsValid(CreatedTarget),
                    TEXT("RenderTarget [{}] failed to create its managed [{}x{}] target."),
                    InRenderTargetEntity, ClampedSize.X, ClampedSize.Y)
                { return nullptr; }

                return CreatedTarget;
            }
        }

        return nullptr;
    }

    // Builds the per-player chunk queue entries for one compressed payload.
    auto
    BuildChunks(
        ECk_RenderTarget_PixelPayloadKind InKind,
        int32 InPayloadSeq,
        const FIntPoint& InSize,
        int32 InUncompressedSize,
        int32 InInstructionWatermark,
        const TArray<uint8>& InBytes) -> TArray<ck::FCk_RenderTarget_PixelChunk>
    {
        const auto RawChunks = ck::render_target::pixel::Chunk_Payload(InBytes, UCk_Utils_RenderTarget_Settings_UE::Get_ChunkSizeBytes());

        auto Chunks = TArray<ck::FCk_RenderTarget_PixelChunk>{};
        Chunks.Reserve(RawChunks.Num());

        for (auto ChunkIdx = 0; ChunkIdx < RawChunks.Num(); ++ChunkIdx)
        {
            Chunks.Emplace(ck::FCk_RenderTarget_PixelChunk{}
                .Set_Kind(InKind)
                .Set_PayloadSeq(InPayloadSeq)
                .Set_ChunkIdx(ChunkIdx)
                .Set_NumChunks(RawChunks.Num())
                .Set_UncompressedSize(InUncompressedSize)
                .Set_Size(InSize)
                .Set_InstructionWatermark(InInstructionWatermark)
                .Set_Bytes(RawChunks[ChunkIdx]));
        }

        return Chunks;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // SETUP
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent)
        -> void
    {
        InRenderTargetEntity.Remove<MarkedDirtyBy>();

        auto* ResolvedTarget = ck_render_target_processor::ResolveDrawableTarget(InRenderTargetEntity, InParams);

        if (ck::Is_NOT_Valid(ResolvedTarget))
        { return; }

        InCurrent._Target = TStrongObjectPtr{ResolvedTarget};

        // Composition anchor for the snapshot restore pass: HydrateFromSavedChannel's view keys on
        // Params + AuthoredLog, so the log must exist on EVERY sync entity from setup — not lazily at
        // first publish — or a never-drawn (or non-publishing) target restores as a half-composed
        // zombie (Params only, no Current/label, unreachable). Empty until a covered author records.
        InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_AuthoredLog>();

        // Attach the owner-entity instruction container on authority (channel A). One container
        // per owner, one channel per sync child. TryAddContainerFragment no-ops when the owner
        // doesn't replicate or has no driver — gating here keeps the local-only fast path free
        // of the driver lookup and makes the intent explicit (SM Setup pattern).
        if (InParams.Get_Replication() == ECk_Replication::Replicates
            && UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity))
        {
            auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);

            if (ck::IsValid(OwnerEntity))
            {
                UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_RenderTarget>(OwnerEntity);

                const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);
                UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_RenderTarget>(
                    OwnerEntity,
                    [&](FCk_RepData_RenderTarget& RepData) -> void
                    {
                        if (RepData.Find_Channel(SyncName) != nullptr)
                        { return; }

                        RepData.Get_Channels().Emplace(
                            FCk_RenderTarget_ChannelState{}.Set_SyncName(SyncName));
                    });
            }
        }

        if (InParams.Get_PixelSyncPolicy() == ECk_RenderTarget_PixelSyncPolicy::Interval)
        {
            if (InParams.Get_SyncInterval() > FCk_Time::ZeroSecond())
            {
                InRenderTargetEntity.Get<FFragment_RenderTarget_PixelSync>()._IntervalChrono =
                    FCk_Chrono{InParams.Get_SyncInterval()};
            }
            else
            {
                CK_TRIGGER_ENSURE(
                    TEXT("RenderTarget [{}] uses the Interval pixel-sync policy with a non-positive _SyncInterval — ")
                    TEXT("interval sync disabled."),
                    InRenderTargetEntity);
            }
        }

        render_target::Verbose(TEXT("RenderTarget [{}] setup complete — target [{}]"),
            InRenderTargetEntity, InCurrent._Target.Get());
    }

    // ================================================================================================================
    // SAVE-LOAD HYDRATION
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_HandleRequests::
        HydrateFromSavedChannel(
            FCk_Handle& InChild,
            const FCk_RenderTarget_ChannelState& InChannel)
        -> bool
    {
        // The v3 hydration payload is CHILD-keyed (Produce reads this sync child's own AuthoredLog,
        // CkRenderTarget_Replication.cpp), so InChild IS the sync child — unlike the net Apply below it,
        // which is owner-keyed (TryGet_RenderTarget on the owner). On the loading AUTHORITY, refill the
        // child's instruction ring + repaint + (Replicates) re-publish into a fresh owner container so
        // post-load clients reconverge through the ordinary ApplyReplicatedBatches path.

        // ---- Preconditions: evaluate ALL before any mutation (exactly-once repaint) ----
        // The fresh v3 Construct re-created this child; its Setup (FGroup_Gameplay_Rendering) runs in the
        // load's Full settle-pump BEFORE FGroup_Hydration, adding Current + resolving the target + removing
        // NeedsSetup. Return NotReady (retry) until it has composed.
        if (NOT InChild.Has<FFragment_RenderTarget_Params>()
            || NOT InChild.Has<FFragment_RenderTarget_Current>()
            || NOT InChild.Has<FFragment_RenderTarget_AuthoredLog>())
        { return false; }

        if (InChild.Has<FTag_RenderTarget_NeedsSetup>())
        { return false; }

        const auto& Params = InChild.Get<FFragment_RenderTarget_Params>();

        // Replicating targets re-publish into a fresh owner container (the owner-hosted FCk_RepData_RenderTarget
        // lives in the replication driver's FastArray, which the snapshot never captures). Gate on the owner's
        // driver — re-established by the snapshot respawn pass — retrying until it exists. DoesNotReplicate
        // targets skip the container half but still restore their drawn state (mode-agnostic, per Produce).
        const auto RepublishToOwner = Params.Get_Replication() == ECk_Replication::Replicates
            && UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InChild);

        auto OwnerEntity = FCk_Handle{};
        if (RepublishToOwner)
        {
            OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InChild);
            if (ck::Is_NOT_Valid(OwnerEntity) || NOT UCk_Utils_EntityReplicationDriver_UE::Has(OwnerEntity))
            { return false; }
        }

        // ---- Mutations: past every NotReady gate, so this runs exactly once per load ----
        auto  RenderTargetEntity = ck::StaticCast<FCk_Handle_RenderTarget>(InChild);
        auto& Current            = InChild.Get<FFragment_RenderTarget_Current>();
        auto& AuthoredLog        = InChild.Get<FFragment_RenderTarget_AuthoredLog>();
        const auto& Batches      = InChannel.Get_Batches();

        // Refill the persisted ring the fresh Setup left empty: a re-save must re-capture the drawn state,
        // and future local draws must continue monotonically past the restored watermark (Get_LatestAppliedBatchSeq
        // reads Current._NextBatchSeq - 1 on the host).
        for (const auto& Batch : Batches)
        { AuthoredLog.Record_PublishedBatch(Batch, Batch.Get_Seq() + 1); }

        if (NOT Batches.IsEmpty())
        { Current._NextBatchSeq = Batches.Last().Get_Seq() + 1; }

        // Repaint the restored target by replaying the ring (no-ops headless / on non-rendering processes).
        for (const auto& Batch : Batches)
        { DoApplyBatch(RenderTargetEntity, Current, Batch.Get_Cmds()); }

        if (RepublishToOwner)
        {
            UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_RenderTarget>(OwnerEntity);

            const auto SyncName = Params.Get_SyncName();
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_RenderTarget>(
                OwnerEntity,
                [&](FCk_RepData_RenderTarget& InRepData) -> void
                {
                    if (InRepData.Find_Channel(SyncName) == nullptr)
                    { InRepData.Get_Channels().Emplace(FCk_RenderTarget_ChannelState{}.Set_SyncName(SyncName)); }

                    auto* Channel = InRepData.Find_Channel(SyncName);
                    Channel->Set_Batches(Batches);

                    if (NOT Batches.IsEmpty())
                    { Channel->Set_LatestSeq(Batches.Last().Get_Seq()); }
                });
        }

        render_target::Verbose(TEXT("RenderTarget [{}] hydrated from saved channel — [{}] batch(es) replayed, next seq [{}]"),
            RenderTargetEntity, Batches.Num(), Current.Get_NextBatchSeq());

        return true;
    }

    // ================================================================================================================
    // HANDLE REQUESTS
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        if (InRequests._Requests.IsEmpty())
        {
            InRenderTargetEntity.Remove<MarkedDirtyBy>();
        }

        if (RequestsCopy.IsEmpty())
        { return; }

        // Single-author gate: draw mutations may only originate on a machine allowed to author
        // this target. DoesNotReplicate targets are self-authoritative everywhere; replicating
        // targets are authored by the host, plus clients when _ClientAuthoring is Allowed
        // (zero-latency prediction; PushClientBatches forwards the batch to the server).
        const auto IsLocalAuthor =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity)
            || InParams.Get_ClientAuthoring() == ECk_RenderTarget_ClientAuthoring::Allowed;

        CK_ENSURE_IF_NOT(IsLocalAuthor,
            TEXT("RenderTarget [{}] received [{}] draw request(s) on a non-authoring client "
                 "(_ClientAuthoring is Disallowed). Dropping the batch — draw on the server, or set "
                 "_ClientAuthoring to Allowed."),
            InRenderTargetEntity, RequestsCopy.Num())
        {
            algo::ForEachRequest(RequestsCopy, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                if (InRequest.Get_IsRequestHandleValid())
                { InRequest.GetAndDestroyRequestHandle(); }
            }), policy::DontResetContainer{});
            return;
        }

        auto Cmds = TArray<FCk_RenderTarget_DrawCmd>{};
        Cmds.Reserve(RequestsCopy.Num());

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InRenderTargetEntity, Cmds, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});

        if (Cmds.IsEmpty())
        { return; }

        DoApplyBatch(InRenderTargetEntity, InCurrent, Cmds);

        const auto BatchSeq = InCurrent._NextBatchSeq;
        InCurrent._NextBatchSeq = BatchSeq + 1;

        // Channel A publication. The host republishes locally-applied batches into the owner's
        // instruction ring; an authoring CLIENT applied the batch locally as prediction and
        // buffers it for the relay push (the server re-applies, assigns its own seq, and
        // republishes with the sender stamped). Pixels-only targets reconcile via the pixel
        // stream and never carry instructions on the wire.
        if (InParams.Get_Replication() == ECk_Replication::Replicates
            && InParams.Get_SyncMode() != ECk_RenderTarget_SyncMode::Pixels)
        {
            if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity))
            {
                DoPublishBatch(InRenderTargetEntity, BatchSeq, Cmds);
            }
            else
            {
                const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);

                auto& Pending = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_PendingClientBatches>();
                Pending._Batches.Emplace(FCk_RenderTarget_InstructionBatch{}
                    .Set_Seq(BatchSeq)
                    .Set_Cmds(Cmds)
                    .Set_Sender(ck_render_target_processor::Get_LocalPlayerState(World)));
            }
        }
        else if (InParams.Get_Replication() == ECk_Replication::DoesNotReplicate)
        {
            // Local-only targets never publish (no DoPublishBatch), but their drawn state must still
            // persist through a snapshot — record the applied batch into the authored log directly.
            // Each world is self-authoritative for DoesNotReplicate; only the server world is ever
            // captured, so recording everywhere is harmless. Replicates+Pixels falls through both
            // branches unrecorded — pixel-baseline persistence is the documented v1 deferral.
            InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_AuthoredLog>()
                .Record_PublishedBatch(
                    FCk_RenderTarget_InstructionBatch{}.Set_Seq(BatchSeq).Set_Cmds(Cmds),
                    BatchSeq + 1);
        }

        UUtils_Signal_RenderTarget_OnInstructionsApplied::Broadcast(InRenderTargetEntity,
            MakePayload(InRenderTargetEntity, BatchSeq, Cmds.Num()));

        render_target::VeryVerbose(TEXT("RenderTarget [{}] applied batch seq [{}] with [{}] cmd(s)"),
            InRenderTargetEntity, BatchSeq, Cmds.Num());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoPublishBatch(
            HandleType InRenderTargetEntity,
            int32 InBatchSeq,
            const TArray<FCk_RenderTarget_DrawCmd>& InCmds,
            APlayerState* InSender)
        -> void
    {
        auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);

        if (ck::Is_NOT_Valid(OwnerEntity))
        { return; }

        const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);

        const auto Batch = FCk_RenderTarget_InstructionBatch{}
            .Set_Seq(InBatchSeq)
            .Set_Cmds(InCmds)
            .Set_Sender(InSender);

        UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_RenderTarget>(
            OwnerEntity,
            [&](FCk_RepData_RenderTarget& RepData) -> void
            {
                auto* Channel = RepData.Find_Channel(SyncName);

                CK_ENSURE_IF_NOT(Channel != nullptr,
                    TEXT("RenderTarget [{}] published a batch but its channel [{}] is not registered in the owner's container"),
                    InRenderTargetEntity, SyncName)
                { return; }

                auto& Batches = Channel->Get_Batches();
                Batches.Emplace(Batch);

                if (Batches.Num() > FCk_RenderTarget_ChannelState::RingSize)
                { Batches.RemoveAt(0); }

                Channel->Set_LatestSeq(InBatchSeq);
            });

        // Mirror the published batch into the snapshotable host-authoritative log on the SYNC CHILD.
        // The channel ring above lives in the replication driver's FastArray, which is re-created
        // empty on a snapshot load and is not itself a snapshotable fragment — so this mirror is the
        // only persistent home for the instruction stream (see FFragment_RenderTarget_AuthoredLog).
        // DoPublishBatch is the single host-side publish site (server draws + applied client batches
        // both flow through it), so the log stays an exact mirror of the channel ring.
        InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_AuthoredLog>()
            .Record_PublishedBatch(Batch, InBatchSeq + 1);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawLine& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Line)
            .Set_PositionA(InRequest.Get_Start())
            .Set_PositionB(InRequest.Get_End())
            .Set_Thickness(InRequest.Get_Thickness())
            .Set_Color(InRequest.Get_Color()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawTexture& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Texture)
            .Set_Asset(InRequest.Get_Texture())
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Size())
            .Set_CoordinatePosition(InRequest.Get_CoordinatePosition())
            .Set_CoordinateSize(InRequest.Get_CoordinateSize())
            .Set_Color(InRequest.Get_Color())
            .Set_Rotation(InRequest.Get_Rotation()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawMaterial& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Material)
            .Set_Asset(InRequest.Get_Material())
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Size())
            .Set_Rotation(InRequest.Get_Rotation()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawText& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Text)
            .Set_Asset(InRequest.Get_Font())
            .Set_Text(InRequest.Get_Text())
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Scale())
            .Set_Color(InRequest.Get_Color()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawBox& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Box)
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Size())
            .Set_Thickness(InRequest.Get_Thickness())
            .Set_Color(InRequest.Get_Color()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawBorder& InRequest)
        -> void
    {
        auto ExtraAssets = TArray<TObjectPtr<UObject>>{};
        ExtraAssets.Emplace(InRequest.Get_BackgroundTexture());
        ExtraAssets.Emplace(InRequest.Get_LeftBorderTexture());
        ExtraAssets.Emplace(InRequest.Get_RightBorderTexture());
        ExtraAssets.Emplace(InRequest.Get_TopBorderTexture());
        ExtraAssets.Emplace(InRequest.Get_BottomBorderTexture());

        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Border)
            .Set_Asset(InRequest.Get_BorderTexture())
            .Set_ExtraAssets(ExtraAssets)
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Size())
            .Set_CoordinatePosition(InRequest.Get_CoordinatePosition())
            .Set_CoordinateSize(InRequest.Get_CoordinateSize())
            .Set_Color(InRequest.Get_Color())
            .Set_Rotation(InRequest.Get_Rotation()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawTriangles& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Triangles)
            .Set_Asset(InRequest.Get_Texture())
            .Set_Triangles(InRequest.Get_Triangles()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_DrawPolygon& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Polygon)
            .Set_Asset(InRequest.Get_Texture())
            .Set_PositionA(InRequest.Get_Position())
            .Set_Size(InRequest.Get_Radius())
            .Set_Sides(InRequest.Get_NumberOfSides())
            .Set_Color(InRequest.Get_Color()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_Clear& InRequest)
        -> void
    {
        InOutCmds.Emplace(FCk_RenderTarget_DrawCmd{}
            .Set_Type(ECk_RenderTarget_DrawCmdType::Clear)
            .Set_Color(InRequest.Get_ClearColor()));
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoHandleRequest(
            HandleType InRenderTargetEntity,
            TArray<FCk_RenderTarget_DrawCmd>& InOutCmds,
            const FCk_Request_RenderTarget_SyncPixels& InRequest)
        -> void
    {
        InRenderTargetEntity.AddOrGet<FTag_RenderTarget_PixelCapturePending>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoApplyBatch(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            const TArray<FCk_RenderTarget_DrawCmd>& InCmds)
        -> void
    {
        if (InCmds.IsEmpty())
        { return; }

        // -nullrhi CI and dedicated servers cannot draw — the batch still counts as applied
        // (seq advances, signal fires) so the instruction stream stays consistent across machines.
        if (NOT FApp::CanEverRender())
        { return; }

        auto* Target = InCurrent.Get_Target().Get();

        // Setup already ensured loudly when the target could not be resolved — stay quiet here so
        // a single misconfiguration doesn't ensure once per batch.
        if (ck::Is_NOT_Valid(Target))
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);

        if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        auto Canvas = static_cast<UCanvas*>(nullptr);
        auto CanvasSize = FVector2D{};
        auto Context = FDrawToRenderTargetContext{};
        auto CanvasIsOpen = false;

        const auto CloseCanvas = [&]() -> void
        {
            if (NOT CanvasIsOpen)
            { return; }

            UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);
            CanvasIsOpen = false;
        };

        for (const auto& Cmd : InCmds)
        {
            // Clear runs outside any canvas pass — it rewrites the whole target.
            if (Cmd.Get_Type() == ECk_RenderTarget_DrawCmdType::Clear)
            {
                CloseCanvas();
                UKismetRenderingLibrary::ClearRenderTarget2D(World, Target, Cmd.Get_Color());
                continue;
            }

            if (NOT CanvasIsOpen)
            {
                UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, Target, Canvas, CanvasSize, Context);

                if (ck::Is_NOT_Valid(Canvas))
                { return; }

                CanvasIsOpen = true;
            }

            DoApplyCmdToCanvas(*Canvas, Cmd);
        }

        CloseCanvas();
    }

    auto
        FProcessor_RenderTarget_HandleRequests::
        DoApplyCmdToCanvas(
            UCanvas& InCanvas,
            const FCk_RenderTarget_DrawCmd& InCmd)
        -> void
    {
        switch (InCmd.Get_Type())
        {
            case ECk_RenderTarget_DrawCmdType::Line:
            {
                InCanvas.K2_DrawLine(InCmd.Get_PositionA(), InCmd.Get_PositionB(),
                    InCmd.Get_Thickness(), InCmd.Get_Color());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Texture:
            {
                auto* Texture = ::Cast<UTexture>(InCmd.Get_Asset());

                CK_ENSURE_IF_NOT(ck::IsValid(Texture),
                    TEXT("DrawTexture cmd has an invalid or non-UTexture asset [{}] — skipping. "
                         "Texture assets must exist on every machine that applies the instruction."),
                    InCmd.Get_Asset())
                { break; }

                InCanvas.K2_DrawTexture(Texture, InCmd.Get_PositionA(), InCmd.Get_Size(),
                    InCmd.Get_CoordinatePosition(), InCmd.Get_CoordinateSize(),
                    InCmd.Get_Color(), BLEND_Translucent, InCmd.Get_Rotation());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Material:
            {
                auto* Material = ::Cast<UMaterialInterface>(InCmd.Get_Asset());

                CK_ENSURE_IF_NOT(ck::IsValid(Material),
                    TEXT("DrawMaterial cmd has an invalid or non-UMaterialInterface asset [{}] — skipping. "
                         "Material assets must exist on every machine that applies the instruction."),
                    InCmd.Get_Asset())
                { break; }

                InCanvas.K2_DrawMaterial(Material, InCmd.Get_PositionA(), InCmd.Get_Size(),
                    FVector2D::ZeroVector, FVector2D::UnitVector, InCmd.Get_Rotation());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Text:
            {
                // Font is optional — fall back to the engine's small font when unset.
                auto* Font = ::Cast<UFont>(InCmd.Get_Asset());

                if (ck::Is_NOT_Valid(Font))
                { Font = GEngine->GetSmallFont(); }

                InCanvas.K2_DrawText(Font, InCmd.Get_Text(), InCmd.Get_PositionA(),
                    InCmd.Get_Size(), InCmd.Get_Color());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Box:
            {
                InCanvas.K2_DrawBox(InCmd.Get_PositionA(), InCmd.Get_Size(),
                    InCmd.Get_Thickness(), InCmd.Get_Color());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Border:
            {
                const auto& Extra = InCmd.Get_ExtraAssets();
                auto* BorderTexture = ::Cast<UTexture>(InCmd.Get_Asset());
                auto* BackgroundTexture = Extra.Num() > 0 ? ::Cast<UTexture>(Extra[0].Get()) : nullptr;
                auto* LeftTexture = Extra.Num() > 1 ? ::Cast<UTexture>(Extra[1].Get()) : nullptr;
                auto* RightTexture = Extra.Num() > 2 ? ::Cast<UTexture>(Extra[2].Get()) : nullptr;
                auto* TopTexture = Extra.Num() > 3 ? ::Cast<UTexture>(Extra[3].Get()) : nullptr;
                auto* BottomTexture = Extra.Num() > 4 ? ::Cast<UTexture>(Extra[4].Get()) : nullptr;

                CK_ENSURE_IF_NOT(ck::IsValid(BorderTexture) && ck::IsValid(BackgroundTexture)
                        && ck::IsValid(LeftTexture) && ck::IsValid(RightTexture)
                        && ck::IsValid(TopTexture) && ck::IsValid(BottomTexture),
                    TEXT("DrawBorder cmd is missing one or more of its six textures — skipping. ")
                    TEXT("Border assets must exist on every machine that applies the instruction."))
                { break; }

                InCanvas.K2_DrawBorder(BorderTexture, BackgroundTexture,
                    LeftTexture, RightTexture, TopTexture, BottomTexture,
                    InCmd.Get_PositionA(), InCmd.Get_Size(),
                    InCmd.Get_CoordinatePosition(), InCmd.Get_CoordinateSize(),
                    InCmd.Get_Color(), FVector2D{0.1f, 0.1f}, FVector2D{0.1f, 0.1f},
                    InCmd.Get_Rotation());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Triangles:
            {
                auto* Texture = ::Cast<UTexture>(InCmd.Get_Asset());

                CK_ENSURE_IF_NOT(ck::IsValid(Texture),
                    TEXT("DrawTriangles cmd has an invalid or non-UTexture asset [{}] — skipping."),
                    InCmd.Get_Asset())
                { break; }

                InCanvas.K2_DrawTriangle(Texture, InCmd.Get_Triangles());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Polygon:
            {
                // Texture is optional — K2_DrawPolygon substitutes the engine's white texture.
                InCanvas.K2_DrawPolygon(::Cast<UTexture>(InCmd.Get_Asset()),
                    InCmd.Get_PositionA(), InCmd.Get_Size(), InCmd.Get_Sides(), InCmd.Get_Color());
                break;
            }
            case ECk_RenderTarget_DrawCmdType::Clear:
            {
                // Handled by DoApplyBatch outside the canvas pass.
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InCmd.Get_Type());
                break;
            }
        }
    }

    // ================================================================================================================
    // INTERVAL SYNC
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_IntervalSync::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        if (InParams.Get_PixelSyncPolicy() != ECk_RenderTarget_PixelSyncPolicy::Interval)
        { return; }

        if (InParams.Get_SyncInterval() <= FCk_Time::ZeroSecond())
        { return; }

        // Interval reconciliation is the AUTHORITY's job — narrower than PixelCapture's
        // authoring gate on purpose. Letting Allowed clients tick the interval makes every
        // authoring client full-upload its board each period (uploads are FullSync-only),
        // ping-ponging state against the host's own reconcile. Clients upload explicitly
        // via Request_SyncPixels.
        const auto IsReconcileAuthority =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity);

        if (NOT IsReconcileAuthority)
        { return; }

        InPixelSync._IntervalChrono.Tick(InDeltaT);

        if (NOT InPixelSync._IntervalChrono.Get_IsDone())
        { return; }

        InPixelSync._IntervalChrono.Reset();
        InRenderTargetEntity.AddOrGet<FTag_RenderTarget_PixelCapturePending>();
    }

    // ================================================================================================================
    // PIXEL CAPTURE
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_PixelCapture::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        InRenderTargetEntity.Remove<MarkedDirtyBy>();

        // Capture authoring gate: the host always captures; clients only for the pixel-upload
        // path (_ClientAuthoring == Allowed). DoesNotReplicate targets are self-authoritative.
        const auto IsCaptureAuthor =
            InParams.Get_Replication() == ECk_Replication::DoesNotReplicate
            || UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity)
            || InParams.Get_ClientAuthoring() == ECk_RenderTarget_ClientAuthoring::Allowed;

        if (NOT IsCaptureAuthor)
        {
            render_target::Warning(
                TEXT("RenderTarget [{}] SyncPixels requested on a non-authoring client — dropped"),
                InRenderTargetEntity);
            return;
        }

        if (NOT FApp::CanEverRender())
        {
            render_target::Warning(
                TEXT("RenderTarget [{}] SyncPixels requested but this process cannot render — dropped. "
                     "Use Debug_InjectCapturedPixels for headless flows."),
                InRenderTargetEntity);
            return;
        }

        auto* Target = InCurrent.Get_Target().Get();

        if (ck::Is_NOT_Valid(Target))
        {
            render_target::Warning(
                TEXT("RenderTarget [{}] SyncPixels requested but no drawable target is set up — dropped"),
                InRenderTargetEntity);
            return;
        }

        auto Readback = MakeShared<FCk_RenderTarget_Readback>();

        if (NOT Readback->Request_EnqueueCopy(Target))
        { return; }

        InPixelSync._Readback = Readback;
        InPixelSync._PendingInstructionWatermark = InCurrent.Get_NextBatchSeq() - 1;
        InRenderTargetEntity.Add<FTag_RenderTarget_PixelSyncInFlight>();

        render_target::VeryVerbose(TEXT("RenderTarget [{}] pixel readback enqueued (watermark [{}])"),
            InRenderTargetEntity, InPixelSync._PendingInstructionWatermark);
    }

    // ================================================================================================================
    // PIXEL SYNC PUMP
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_PixelSyncPump::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        if (InPixelSync._JobResult.IsValid())
        {
            if (NOT InPixelSync._JobResult->_Done)
            { return; }

            DoFinishPass(InRenderTargetEntity, InPixelSync);
            return;
        }

        if (NOT InPixelSync._Readback.IsValid())
        {
            // Neither readback nor job — stale in-flight tag (should not happen); recover.
            CK_TRIGGER_ENSURE(TEXT("RenderTarget [{}] is tagged PixelSyncInFlight with no readback or job"),
                InRenderTargetEntity);
            InRenderTargetEntity.Remove<FTag_RenderTarget_PixelSyncInFlight>();
            return;
        }

        switch (InPixelSync._Readback->Tick())
        {
            case ECk_RenderTarget_ReadbackState::Complete:
            {
                auto Pixels = TArray<uint8>{};
                auto Size = FIntPoint::ZeroValue;

                if (NOT InPixelSync._Readback->TakePixels(Pixels, Size))
                {
                    InPixelSync._Readback.Reset();
                    InRenderTargetEntity.Remove<FTag_RenderTarget_PixelSyncInFlight>();
                    return;
                }

                InPixelSync._Readback.Reset();

                // Client upload captures always produce a FullSync (empty previous forces it):
                // the server's snapshot may have diverged (other authors interleave), so a delta
                // diffed against THIS client's history would not patch cleanly there.
                const auto IsUploadBound =
                    InRenderTargetEntity.Get<FFragment_RenderTarget_Params>().Get_Replication() == ECk_Replication::Replicates
                    && NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity);

                InPixelSync._JobResult = render_target::pixel::Launch_DiffAndCompressJob(
                    MoveTemp(Pixels), Size,
                    IsUploadBound ? TArray<uint8>{} : InPixelSync._LastSyncedSnapshot,
                    IsUploadBound ? FIntPoint::ZeroValue : InPixelSync._SnapshotSize,
                    UCk_Utils_RenderTarget_Settings_UE::Get_BlockSize());
                break;
            }
            case ECk_RenderTarget_ReadbackState::Failed:
            {
                render_target::Warning(TEXT("RenderTarget [{}] pixel readback failed — pass dropped"),
                    InRenderTargetEntity);
                InPixelSync._Readback.Reset();
                InRenderTargetEntity.Remove<FTag_RenderTarget_PixelSyncInFlight>();
                break;
            }
            default:
            { break; }
        }
    }

    auto
        FProcessor_RenderTarget_PixelSyncPump::
        DoFinishPass(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        const auto JobResult = InPixelSync._JobResult;
        InPixelSync._JobResult.Reset();
        InRenderTargetEntity.Remove<FTag_RenderTarget_PixelSyncInFlight>();

        if (JobResult->_ZeroDiff)
        {
            render_target::VeryVerbose(TEXT("RenderTarget [{}] pixel pass produced zero diff — dropped"),
                InRenderTargetEntity);
            return;
        }

        CK_ENSURE_IF_NOT(NOT JobResult->_CompressedBytes.IsEmpty(),
            TEXT("RenderTarget [{}] pixel pass produced an empty payload — dropped"),
            InRenderTargetEntity)
        { return; }

        InPixelSync._LastSyncedSnapshot = MoveTemp(JobResult->_NewSnapshot);
        InPixelSync._SnapshotSize = JobResult->_Size;
        InPixelSync._SnapshotInstructionWatermark = InPixelSync._PendingInstructionWatermark;

        const auto PayloadSeq = InPixelSync._NextPayloadSeq;
        InPixelSync._NextPayloadSeq = PayloadSeq + 1;

        auto& Payload = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_PendingPixelPayload>();
        Payload._Kind = JobResult->_Kind;
        Payload._PayloadSeq = PayloadSeq;
        Payload._Size = JobResult->_Size;
        Payload._UncompressedSize = JobResult->_UncompressedSize;
        Payload._Bytes = MoveTemp(JobResult->_CompressedBytes);
        Payload._InstructionWatermark = InPixelSync._PendingInstructionWatermark;

        INC_DWORD_STAT(STAT_CkRenderTarget_PayloadsProduced);

        UUtils_Signal_RenderTarget_OnPixelPayloadProduced::Broadcast(InRenderTargetEntity,
            MakePayload(InRenderTargetEntity, Payload.Get_Kind(), PayloadSeq));

        render_target::Verbose(
            TEXT("RenderTarget [{}] produced pixel payload seq [{}] kind [{}] — [{}] compressed bytes ([{}] raw)"),
            InRenderTargetEntity, PayloadSeq, Payload.Get_Kind(), Payload.Get_Bytes().Num(), Payload.Get_UncompressedSize());
    }

    // ================================================================================================================
    // FLUSH PENDING REPLICATION
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_FlushPendingReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PendingReplicationBatches& InStash)
        -> void
    {
        if (NOT InStash._Stash.IsEmpty())
        {
            auto& Queue = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_ReplayQueue>().Get_Queue();
            Queue.Append(MoveTemp(InStash._Stash));

            render_target::VeryVerbose(TEXT("RenderTarget [{}] released [{}] stashed batch(es) to the replay queue"),
                InRenderTargetEntity, Queue.Num());
        }

        InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingReplicationBatches>();
    }

    // ================================================================================================================
    // APPLY REPLICATED BATCHES
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_ApplyReplicatedBatches::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ReplayQueue& InReplayQueue)
        -> void
    {
        const auto QueueCopy = InReplayQueue._Queue;
        InReplayQueue._Queue.Reset();

        if (InReplayQueue._Queue.IsEmpty())
        {
            InRenderTargetEntity.Remove<MarkedDirtyBy>();
        }

        if (QueueCopy.IsEmpty())
        { return; }

        auto& ReplayState = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_ClientReplayState>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);
        const auto* LocalPlayer = ck_render_target_processor::Get_LocalPlayerState(World);

        for (const auto& Batch : QueueCopy)
        {
            const auto Seq = Batch.Get_Seq();

            // Already applied, or baked into a pixel baseline this world received.
            if (Seq <= ReplayState._LastAppliedSeq || Seq <= ReplayState._BaselineInstructionWatermark)
            { continue; }

            // Echo suppression: this world authored the batch and applied it at request time
            // (prediction). Advance the watermark WITHOUT re-applying — skipping silently would
            // leave a seq hole that falsely trips the ring-wrap gap recovery below.
            if (Batch.Get_Sender().IsValid() && Batch.Get_Sender().Get() == LocalPlayer)
            {
                ReplayState._LastAppliedSeq = Seq;
                continue;
            }

            // Ring-wrap gap: instructions between the watermark and this batch aged out of the
            // ring before this world saw them. Apply what we have (best-effort) and flag for a
            // pixel-baseline reconcile — ClientNetMaintenance turns the tag into Server_RequestFullSync.
            if (Seq > ReplayState._LastAppliedSeq + 1 && ReplayState._LastAppliedSeq > 0)
            {
                render_target::Warning(
                    TEXT("RenderTarget [{}] instruction gap — last applied [{}], next available [{}]. ")
                    TEXT("Lost batches aged out of the ring; flagging for pixel baseline."),
                    InRenderTargetEntity, ReplayState._LastAppliedSeq, Seq);
                InRenderTargetEntity.AddOrGet<FTag_RenderTarget_NeedsBaseline>();
            }

            FProcessor_RenderTarget_HandleRequests::DoApplyBatch(InRenderTargetEntity, InCurrent, Batch.Get_Cmds());
            ReplayState._LastAppliedSeq = Seq;

            UUtils_Signal_RenderTarget_OnInstructionsApplied::Broadcast(InRenderTargetEntity,
                MakePayload(InRenderTargetEntity, Seq, Batch.Get_Cmds().Num()));

            render_target::VeryVerbose(TEXT("RenderTarget [{}] replayed batch seq [{}] with [{}] cmd(s)"),
                InRenderTargetEntity, Seq, Batch.Get_Cmds().Num());
        }
    }

    // ================================================================================================================
    // DISPATCH PIXEL PAYLOAD
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_DispatchPixelPayload::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_PendingPixelPayload& InPayload)
        -> void
    {
        const auto Payload = InPayload;
        InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingPixelPayload>();

        if (InParams.Get_Replication() == ECk_Replication::DoesNotReplicate)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);

        if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto* GameState = World->GetGameState();

        if (ck::Is_NOT_Valid(GameState, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        const auto Chunks = ck_render_target_processor::BuildChunks(
            Payload.Get_Kind(), Payload.Get_PayloadSeq(), Payload.Get_Size(),
            Payload.Get_UncompressedSize(), Payload.Get_InstructionWatermark(), Payload.Get_Bytes());

        auto& Streams = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_HostStreams>();

        for (const auto& PlayerPtr : GameState->PlayerArray)
        {
            auto* Player = PlayerPtr.Get();

            if (ck::Is_NOT_Valid(Player))
            { continue; }

            // The listen-server host applied the pixels locally at capture time — and a Client
            // RPC on a locally-owned channel would execute locally, double-applying.
            if (const auto* Controller = Player->GetPlayerController();
                ck::IsValid(Controller, ck::IsValid_Policy_NullptrOnly{}) && Controller->IsLocalController())
            { continue; }

            auto& Stream = Streams._Streams.FindOrAdd(Player);

            // The payload re-broadcasts this player's own upload — they already have the pixels
            // (they authored them). Skip the stream and promote their baseline instead.
            if (Payload.Get_ExcludePlayer().IsValid() && Payload.Get_ExcludePlayer().Get() == Player)
            {
                if (NOT Stream._HasBaseline)
                {
                    Stream._HasBaseline = true;

                    UUtils_Signal_RenderTarget_OnClientBaselineEstablished::Broadcast(InRenderTargetEntity,
                        MakePayload(InRenderTargetEntity, TWeakObjectPtr<APlayerState>{Player}));
                }
                continue;
            }

            // A delta is meaningless without the baseline it patches — flag for an on-demand
            // FullSync instead (late joiners always get a FullSync first).
            if (Payload.Get_Kind() == ECk_RenderTarget_PixelPayloadKind::Delta && NOT Stream._HasBaseline)
            {
                Stream._NeedsFullSync = true;
                continue;
            }

            Stream._Chunks.Append(Chunks);
            Stream._LastSentPayloadSeq = Payload.Get_PayloadSeq();
        }
    }

    // ================================================================================================================
    // PACE STREAMS
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_PaceStreams::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        DoDrainInbox(InRenderTargetEntity, InStreams);
        DoManageBaselineJob(InRenderTargetEntity, InStreams, InPixelSync);
        DoSendChunks(InRenderTargetEntity, InStreams);
    }

    auto
        FProcessor_RenderTarget_PaceStreams::
        DoDrainInbox(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams)
        -> void
    {
        if (NOT InRenderTargetEntity.Has<FFragment_RenderTarget_HostInbox>())
        { return; }

        const auto Requests = InRenderTargetEntity.Get<FFragment_RenderTarget_HostInbox>().Get_Requests();
        InRenderTargetEntity.Remove<FFragment_RenderTarget_HostInbox>();

        for (const auto& Request : Requests)
        {
            auto* Player = Request.Get_Player().Get();

            if (ck::Is_NOT_Valid(Player))
            { continue; }

            auto& Stream = InStreams._Streams.FindOrAdd(Player);

            switch (Request.Get_Kind())
            {
                case ECk_RenderTarget_StreamRequestKind::AckFullSync:
                {
                    if (NOT Stream._HasBaseline)
                    {
                        Stream._HasBaseline = true;

                        UUtils_Signal_RenderTarget_OnClientBaselineEstablished::Broadcast(InRenderTargetEntity,
                            MakePayload(InRenderTargetEntity, TWeakObjectPtr<APlayerState>{Player}));

                        render_target::Verbose(TEXT("RenderTarget [{}] baseline established for [{}] (payload [{}])"),
                            InRenderTargetEntity, Player, Request.Get_PayloadSeq());
                    }
                    break;
                }
                case ECk_RenderTarget_StreamRequestKind::RequestFullSync:
                {
                    Stream._NeedsFullSync = true;
                    Stream._HasBaseline = false;
                    break;
                }
            }
        }
    }

    auto
        FProcessor_RenderTarget_PaceStreams::
        DoManageBaselineJob(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams,
            FFragment_RenderTarget_PixelSync& InPixelSync)
        -> void
    {
        if (InStreams._BaselineJob.IsValid())
        {
            if (NOT InStreams._BaselineJob->_Done)
            { return; }

            const auto JobResult = InStreams._BaselineJob;
            InStreams._BaselineJob.Reset();

            if (JobResult->_CompressedBytes.IsEmpty())
            {
                render_target::Warning(TEXT("RenderTarget [{}] on-demand baseline compress produced no payload"),
                    InRenderTargetEntity);
                return;
            }

            const auto PayloadSeq = InPixelSync._NextPayloadSeq;
            InPixelSync._NextPayloadSeq = PayloadSeq + 1;

            const auto Chunks = ck_render_target_processor::BuildChunks(
                ECk_RenderTarget_PixelPayloadKind::FullSync, PayloadSeq, JobResult->_Size,
                JobResult->_UncompressedSize, InStreams._BaselineJobInstructionWatermark,
                JobResult->_CompressedBytes);

            for (auto& [Player, Stream] : InStreams._Streams)
            {
                if (NOT Stream._NeedsFullSync)
                { continue; }

                Stream._NeedsFullSync = false;
                Stream._Chunks.Append(Chunks);
                Stream._LastSentPayloadSeq = PayloadSeq;
            }

            return;
        }

        const auto AnyStreamNeedsFullSync = ck::algo::AnyOf(InStreams._Streams,
            [](const auto& InKvp) -> bool { return InKvp.Value._NeedsFullSync; });

        if (NOT AnyStreamNeedsFullSync)
        { return; }

        if (InPixelSync._LastSyncedSnapshot.IsEmpty())
        {
            // Nothing has ever been captured — there is no baseline to give. The flags persist;
            // the first capture's payload (a FullSync, since there's no snapshot) serves them.
            return;
        }

        // Empty "previous" forces the FullSync path in the shared job.
        InStreams._BaselineJobInstructionWatermark = InPixelSync._SnapshotInstructionWatermark;
        InStreams._BaselineJob = render_target::pixel::Launch_DiffAndCompressJob(
            InPixelSync._LastSyncedSnapshot, InPixelSync._SnapshotSize,
            TArray<uint8>{}, FIntPoint::ZeroValue,
            UCk_Utils_RenderTarget_Settings_UE::Get_BlockSize());
    }

    auto
        FProcessor_RenderTarget_PaceStreams::
        DoSendChunks(
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_HostStreams& InStreams)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);
        auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);
        const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);
        const auto* OwningActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor_Recursive(InRenderTargetEntity);

        for (auto StreamIt = InStreams._Streams.CreateIterator(); StreamIt; ++StreamIt)
        {
            auto* Player = StreamIt->Key.Get();

            if (ck::Is_NOT_Valid(Player))
            {
                StreamIt.RemoveCurrent();
                continue;
            }

            auto& Stream = StreamIt->Value;

            // Cull-distance relevancy: players beyond the owning actor's
            // NetCullDistanceSquared are skipped and their baseline invalidated — on re-entry
            // the no-baseline path forces a FullSync before any deltas.
            if (ck::IsValid(OwningActor, ck::IsValid_Policy_NullptrOnly{})
                && OwningActor->GetNetCullDistanceSquared() > 0.0f)
            {
                if (const auto* PlayerPawn = Player->GetPawn();
                    ck::IsValid(PlayerPawn, ck::IsValid_Policy_NullptrOnly{}))
                {
                    const auto DistSq = FVector::DistSquared(
                        PlayerPawn->GetActorLocation(), OwningActor->GetActorLocation());

                    if (DistSq > OwningActor->GetNetCullDistanceSquared())
                    {
                        if (Stream._HasBaseline || NOT Stream._Chunks.IsEmpty())
                        {
                            render_target::Verbose(
                                TEXT("RenderTarget [{}] player [{}] left cull range — baseline invalidated, [{}] queued chunk(s) dropped"),
                                InRenderTargetEntity, Player, Stream._Chunks.Num());
                        }

                        Stream._HasBaseline = false;
                        Stream._NeedsFullSync = false;
                        Stream._Chunks.Reset();
                        continue;
                    }
                }
            }

            if (Stream._Chunks.IsEmpty())
            { continue; }

            auto* Channel = ck_render_target_processor::ResolveRelayChannel_ForPlayer(World, Player);

            if (ck::Is_NOT_Valid(Channel))
            { continue; }

            auto BudgetBytes = UCk_Utils_RenderTarget_Settings_UE::Get_MaxBytesPerStreamPerTick();

            while (NOT Stream._Chunks.IsEmpty() && BudgetBytes > 0)
            {
                const auto& Chunk = Stream._Chunks[0];

                Channel->Client_ReceivePixelChunk(
                    OwnerEntity, SyncName,
                    Chunk.Get_Kind(), Chunk.Get_PayloadSeq(), Chunk.Get_ChunkIdx(), Chunk.Get_NumChunks(),
                    Chunk.Get_UncompressedSize(), Chunk.Get_Size(), Chunk.Get_InstructionWatermark(),
                    Chunk.Get_Bytes());

                INC_DWORD_STAT_BY(STAT_CkRenderTarget_PixelBytesSent, Chunk.Get_Bytes().Num());

                BudgetBytes -= Chunk.Get_Bytes().Num();
                Stream._Chunks.RemoveAt(0);
            }
        }
    }

    // ================================================================================================================
    // FLUSH OWNER CHUNK STASH
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_FlushOwnerChunkStash::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InOwnerEntity,
            FFragment_RenderTarget_OwnerChunkStash& InStash)
        -> void
    {
        auto& Stashed = InStash.Get_StashedChunks();

        for (auto Index = 0; Index < Stashed.Num(); /* conditional advance */)
        {
            auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(InOwnerEntity, Stashed[Index].Get_SyncName());

            if (ck::Is_NOT_Valid(SyncEntity))
            {
                ++Index;
                continue;
            }

            SyncEntity.AddOrGet<FFragment_RenderTarget_ChunkInbox>().Get_Chunks().Emplace(Stashed[Index].Get_Chunk());
            Stashed.RemoveAt(Index);
        }

        if (Stashed.IsEmpty())
        {
            InOwnerEntity.Remove<FFragment_RenderTarget_OwnerChunkStash>();
        }
    }

    // ================================================================================================================
    // RECEIVE PIXELS
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_ReceivePixels::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging)
        -> void
    {
        if (InStaging._ApplyJob.IsValid())
        {
            if (NOT InStaging._ApplyJob->_Done)
            { return; }

            DoFinishApply(InRenderTargetEntity, InCurrent, InStaging);
        }

        if (InRenderTargetEntity.Has<FFragment_RenderTarget_ChunkInbox>())
        {
            auto& Inbox = InRenderTargetEntity.Get<FFragment_RenderTarget_ChunkInbox>().Get_Chunks();
            InStaging._Assembling.Append(MoveTemp(Inbox));
            InRenderTargetEntity.Remove<FFragment_RenderTarget_ChunkInbox>();
        }

        if (InStaging._Assembling.IsEmpty() || InStaging._ApplyJob.IsValid())
        { return; }

        // Reliable+ordered delivery means the head of _Assembling is the current payload, in
        // chunk order. Collect its prefix; kick the apply job once all chunks are present.
        const auto HeadSeq = InStaging._Assembling[0].Get_PayloadSeq();
        const auto HeadNumChunks = InStaging._Assembling[0].Get_NumChunks();

        auto HeadChunkCount = 0;
        while (HeadChunkCount < InStaging._Assembling.Num()
            && InStaging._Assembling[HeadChunkCount].Get_PayloadSeq() == HeadSeq)
        {
            CK_ENSURE_IF_NOT(InStaging._Assembling[HeadChunkCount].Get_ChunkIdx() == HeadChunkCount,
                TEXT("RenderTarget [{}] chunk order violation in payload [{}] — expected idx [{}], got [{}]. ")
                TEXT("Dropping the payload."),
                InRenderTargetEntity, HeadSeq, HeadChunkCount, InStaging._Assembling[HeadChunkCount].Get_ChunkIdx())
            {
                InStaging._Assembling.RemoveAll([&](const FCk_RenderTarget_PixelChunk& InChunk) -> bool
                {
                    return InChunk.Get_PayloadSeq() == HeadSeq;
                });
                return;
            }
            ++HeadChunkCount;
        }

        if (HeadChunkCount < HeadNumChunks)
        { return; }

        auto PayloadChunks = TArray<TArray<uint8>>{};
        PayloadChunks.Reserve(HeadNumChunks);
        for (auto ChunkIdx = 0; ChunkIdx < HeadNumChunks; ++ChunkIdx)
        {
            PayloadChunks.Emplace(InStaging._Assembling[ChunkIdx].Get_Bytes());
        }

        const auto& Head = InStaging._Assembling[0];
        InStaging._ApplyJobKind = Head.Get_Kind();
        InStaging._ApplyJobPayloadSeq = Head.Get_PayloadSeq();
        InStaging._ApplyJobInstructionWatermark = Head.Get_InstructionWatermark();
        InStaging._ApplyJob = render_target::pixel::Launch_DecompressAndApplyJob(
            Head.Get_Kind(),
            render_target::pixel::Reassemble_Chunks(PayloadChunks),
            Head.Get_UncompressedSize(),
            Head.Get_Size(),
            InStaging._Pixels,
            InStaging._Size,
            UCk_Utils_RenderTarget_Settings_UE::Get_BlockSize());

        InStaging._Assembling.RemoveAt(0, HeadNumChunks);
    }

    auto
        FProcessor_RenderTarget_ReceivePixels::
        DoFinishApply(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging)
        -> void
    {
        const auto JobResult = InStaging._ApplyJob;
        InStaging._ApplyJob.Reset();

        if (JobResult->_Failed)
        {
            render_target::Warning(
                TEXT("RenderTarget [{}] pixel payload [{}] failed to apply — requesting a fresh baseline"),
                InRenderTargetEntity, InStaging._ApplyJobPayloadSeq);
            InRenderTargetEntity.AddOrGet<FTag_RenderTarget_NeedsBaseline>();
            return;
        }

        InStaging._Pixels = MoveTemp(JobResult->_NewStaging);
        InStaging._Size = JobResult->_Size;

        if (InStaging._ApplyJobKind == ECk_RenderTarget_PixelPayloadKind::FullSync)
        {
            // Instructions at or below the baseline's watermark are baked into these pixels —
            // drop them from the replay paths and advance the applied watermark.
            const auto Watermark = InStaging._ApplyJobInstructionWatermark;

            auto& ReplayState = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_ClientReplayState>();
            ReplayState._BaselineInstructionWatermark = Watermark;

            if (ReplayState._LastAppliedSeq < Watermark)
            { ReplayState._LastAppliedSeq = Watermark; }

            const auto IsBakedIn = [&](const FCk_RenderTarget_InstructionBatch& InBatch) -> bool
            {
                return InBatch.Get_Seq() <= Watermark;
            };

            if (InRenderTargetEntity.Has<FFragment_RenderTarget_ReplayQueue>())
            { InRenderTargetEntity.Get<FFragment_RenderTarget_ReplayQueue>().Get_Queue().RemoveAll(IsBakedIn); }

            if (InRenderTargetEntity.Has<FFragment_RenderTarget_PendingReplicationBatches>())
            { InRenderTargetEntity.Get<FFragment_RenderTarget_PendingReplicationBatches>().Get_Stash().RemoveAll(IsBakedIn); }

            InRenderTargetEntity.Try_Remove<FTag_RenderTarget_NeedsBaseline>();
            InRenderTargetEntity.Try_Remove<FTag_RenderTarget_BaselineRequested>();

            InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_PendingAck>().Set_PayloadSeq(InStaging._ApplyJobPayloadSeq);
        }

        InStaging._LastAppliedPayloadKind = InStaging._ApplyJobKind;
        InStaging._LastAppliedPayloadSeq = InStaging._ApplyJobPayloadSeq;

        DoUploadStagingToTarget(InRenderTargetEntity, InCurrent, InStaging);

        UUtils_Signal_RenderTarget_OnPixelPayloadApplied::Broadcast(InRenderTargetEntity,
            MakePayload(InRenderTargetEntity, InStaging._ApplyJobKind, InStaging._ApplyJobPayloadSeq));

        render_target::Verbose(TEXT("RenderTarget [{}] applied pixel payload seq [{}] kind [{}]"),
            InRenderTargetEntity, InStaging._ApplyJobPayloadSeq, InStaging._ApplyJobKind);
    }

    auto
        FProcessor_RenderTarget_ReceivePixels::
        DoUploadStagingToTarget(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ClientStaging& InStaging)
        -> void
    {
        ck_render_target_processor::DrawPixelsToTarget(
            InRenderTargetEntity, InCurrent, InStaging._Pixels, InStaging._Size, InStaging._UploadTexture);
    }

    // ================================================================================================================
    // CLIENT NET MAINTENANCE
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_ClientNetMaintenance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_ClientStaging& InStaging)
        -> void
    {
        if (InParams.Get_Replication() == ECk_Replication::DoesNotReplicate)
        { return; }

        // A locally-produced pixel payload on a client is an UPLOAD — chunk it for the paced
        // Server_PushPixelChunk sends below. (The authority's payloads are consumed by
        // DispatchPixelPayload instead, which never runs on clients.)
        if (InRenderTargetEntity.Has<FFragment_RenderTarget_PendingPixelPayload>())
        {
            const auto& Payload = InRenderTargetEntity.Get<FFragment_RenderTarget_PendingPixelPayload>();

            CK_ENSURE_IF_NOT(InParams.Get_ClientAuthoring() == ECk_RenderTarget_ClientAuthoring::Allowed,
                TEXT("RenderTarget [{}] produced an upload payload but _ClientAuthoring is Disallowed — dropped"),
                InRenderTargetEntity)
            {
                InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingPixelPayload>();
                return;
            }

            InStaging._UploadChunks.Append(ck_render_target_processor::BuildChunks(
                Payload.Get_Kind(), Payload.Get_PayloadSeq(), Payload.Get_Size(),
                Payload.Get_UncompressedSize(), Payload.Get_InstructionWatermark(), Payload.Get_Bytes()));

            InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingPixelPayload>();
        }

        const auto HasPendingAck = InRenderTargetEntity.Has<FFragment_RenderTarget_PendingAck>();
        const auto NeedsBaselineRequest =
            InRenderTargetEntity.Has<FTag_RenderTarget_NeedsBaseline>()
            && NOT InRenderTargetEntity.Has<FTag_RenderTarget_BaselineRequested>();
        const auto HasUploadChunks = NOT InStaging._UploadChunks.IsEmpty();

        if (NOT HasPendingAck && NOT NeedsBaselineRequest && NOT HasUploadChunks)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);
        auto* LocalPlayer = ck_render_target_processor::Get_LocalPlayerState(World);
        auto* Channel = ck_render_target_processor::ResolveRelayChannel_ForPlayer(World, LocalPlayer);

        if (ck::Is_NOT_Valid(Channel))
        { return; }

        auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);
        const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);

        if (HasPendingAck)
        {
            const auto PayloadSeq = InRenderTargetEntity.Get<FFragment_RenderTarget_PendingAck>().Get_PayloadSeq();
            InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingAck>();
            Channel->Server_AckFullSync(OwnerEntity, SyncName, PayloadSeq);
        }

        if (NeedsBaselineRequest)
        {
            InRenderTargetEntity.Add<FTag_RenderTarget_BaselineRequested>();
            Channel->Server_RequestFullSync(OwnerEntity, SyncName);
        }

        auto UploadBudgetBytes = UCk_Utils_RenderTarget_Settings_UE::Get_MaxBytesPerStreamPerTick();

        while (NOT InStaging._UploadChunks.IsEmpty() && UploadBudgetBytes > 0)
        {
            const auto& Chunk = InStaging._UploadChunks[0];

            Channel->Server_PushPixelChunk(
                OwnerEntity, SyncName,
                Chunk.Get_Kind(), Chunk.Get_PayloadSeq(), Chunk.Get_ChunkIdx(), Chunk.Get_NumChunks(),
                Chunk.Get_UncompressedSize(), Chunk.Get_Size(), Chunk.Get_InstructionWatermark(),
                Chunk.Get_Bytes());

            UploadBudgetBytes -= Chunk.Get_Bytes().Num();
            InStaging._UploadChunks.RemoveAt(0);
        }
    }

    // ================================================================================================================
    // PUSH CLIENT BATCHES
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_PushClientBatches::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            FFragment_RenderTarget_PendingClientBatches& InPending)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);
        auto* LocalPlayer = ck_render_target_processor::Get_LocalPlayerState(World);
        auto* Channel = ck_render_target_processor::ResolveRelayChannel_ForPlayer(World, LocalPlayer);

        // No channel yet — keep the batches and retry next tick (fragment is the retry marker).
        if (ck::Is_NOT_Valid(Channel))
        { return; }

        auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);
        const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);

        for (const auto& Batch : InPending._Batches)
        {
            Channel->Server_PushDrawBatch(OwnerEntity, SyncName, Batch);
        }

        render_target::VeryVerbose(TEXT("RenderTarget [{}] pushed [{}] predicted batch(es) to the server"),
            InRenderTargetEntity, InPending._Batches.Num());

        InRenderTargetEntity.Remove<FFragment_RenderTarget_PendingClientBatches>();
    }

    // ================================================================================================================
    // APPLY CLIENT BATCHES
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_ApplyClientBatches::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_ServerIngressBatches& InIngress)
        -> void
    {
        const auto IngressCopy = InIngress._Batches;
        InRenderTargetEntity.Remove<FFragment_RenderTarget_ServerIngressBatches>();

        if (IngressCopy.IsEmpty())
        { return; }

        CK_ENSURE_IF_NOT(InParams.Get_ClientAuthoring() == ECk_RenderTarget_ClientAuthoring::Allowed,
            TEXT("RenderTarget [{}] received [{}] client-pushed draw batch(es) but _ClientAuthoring is Disallowed — ")
            TEXT("rejected server-side."),
            InRenderTargetEntity, IngressCopy.Num())
        { return; }

        for (const auto& Batch : IngressCopy)
        {
            FProcessor_RenderTarget_HandleRequests::DoApplyBatch(InRenderTargetEntity, InCurrent, Batch.Get_Cmds());

            const auto BatchSeq = InCurrent._NextBatchSeq;
            InCurrent._NextBatchSeq = BatchSeq + 1;

            if (InParams.Get_SyncMode() != ECk_RenderTarget_SyncMode::Pixels)
            {
                FProcessor_RenderTarget_HandleRequests::DoPublishBatch(
                    InRenderTargetEntity, BatchSeq, Batch.Get_Cmds(), Batch.Get_Sender().Get());
            }

            UUtils_Signal_RenderTarget_OnInstructionsApplied::Broadcast(InRenderTargetEntity,
                MakePayload(InRenderTargetEntity, BatchSeq, Batch.Get_Cmds().Num()));

            render_target::VeryVerbose(
                TEXT("RenderTarget [{}] applied client batch from [{}] as seq [{}] with [{}] cmd(s)"),
                InRenderTargetEntity, Batch.Get_Sender().Get(), BatchSeq, Batch.Get_Cmds().Num());
        }
    }

    // ================================================================================================================
    // RECEIVE CLIENT UPLOADS
    // ================================================================================================================

    auto
        FProcessor_RenderTarget_ReceiveClientUploads::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Params& InParams,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync,
            FFragment_RenderTarget_UploadAssembly& InAssembly)
        -> void
    {
        if (InAssembly._ApplyJob.IsValid())
        {
            if (NOT InAssembly._ApplyJob->_Done)
            { return; }

            DoFinishUploadApply(InRenderTargetEntity, InCurrent, InPixelSync, InAssembly);
        }

        if (InAssembly._Inbox.IsEmpty())
        {
            if (NOT InAssembly._ApplyJob.IsValid())
            { InRenderTargetEntity.Remove<FFragment_RenderTarget_UploadAssembly>(); }
            return;
        }

        CK_ENSURE_IF_NOT(InParams.Get_ClientAuthoring() == ECk_RenderTarget_ClientAuthoring::Allowed,
            TEXT("RenderTarget [{}] received [{}] uploaded pixel chunk(s) but _ClientAuthoring is Disallowed — ")
            TEXT("rejected server-side."),
            InRenderTargetEntity, InAssembly._Inbox.Num())
        {
            InAssembly._Inbox.Reset();
            return;
        }

        if (InAssembly._ApplyJob.IsValid())
        { return; }

        // Collect the head sender's prefix for the head payload — reliable per-sender ordering
        // means a sender's chunks arrive in idx order, though chunks of different senders may
        // interleave in the inbox.
        const auto HeadSender = InAssembly._Inbox[0].Get_Sender();
        const auto HeadSeq = InAssembly._Inbox[0].Get_Chunk().Get_PayloadSeq();
        const auto HeadNumChunks = InAssembly._Inbox[0].Get_Chunk().Get_NumChunks();
        const auto& HeadChunk = InAssembly._Inbox[0].Get_Chunk();

        // v1 uploads are FullSync-only (a delta upload would diff against the uploader's own
        // history, which may have diverged from the server snapshot) and size-capped.
        const auto IsValidUpload =
            HeadChunk.Get_Kind() == ECk_RenderTarget_PixelPayloadKind::FullSync
            && HeadChunk.Get_Size().X > 0 && HeadChunk.Get_Size().Y > 0
            && HeadChunk.Get_Size().X <= UCk_Utils_RenderTarget_Settings_UE::Get_MaxManagedSize()
            && HeadChunk.Get_Size().Y <= UCk_Utils_RenderTarget_Settings_UE::Get_MaxManagedSize()
            && HeadChunk.Get_UncompressedSize() ==
                HeadChunk.Get_Size().X * HeadChunk.Get_Size().Y * render_target::pixel::BytesPerPixel;

        auto HeadPayloadChunks = TArray<TArray<uint8>>{};
        auto Collected = 0;

        for (auto Index = 0; Index < InAssembly._Inbox.Num() && Collected < HeadNumChunks; ++Index)
        {
            const auto& Entry = InAssembly._Inbox[Index];

            if (Entry.Get_Sender() != HeadSender || Entry.Get_Chunk().Get_PayloadSeq() != HeadSeq)
            { continue; }

            HeadPayloadChunks.Emplace(Entry.Get_Chunk().Get_Bytes());
            ++Collected;
        }

        if (Collected < HeadNumChunks)
        { return; }

        const auto RemoveHeadPayloadChunks = [&]() -> void
        {
            InAssembly._Inbox.RemoveAll([&](const FCk_RenderTarget_UploadChunk& InEntry) -> bool
            {
                return InEntry.Get_Sender() == HeadSender
                    && InEntry.Get_Chunk().Get_PayloadSeq() == HeadSeq;
            });
        };

        CK_ENSURE_IF_NOT(IsValidUpload,
            TEXT("RenderTarget [{}] rejected a malformed upload from [{}] — kind [{}], size [{}x{}], raw [{}] bytes. ")
            TEXT("v1 uploads must be FullSync, within the managed size cap."),
            InRenderTargetEntity, HeadSender.Get(), HeadChunk.Get_Kind(),
            HeadChunk.Get_Size().X, HeadChunk.Get_Size().Y, HeadChunk.Get_UncompressedSize())
        {
            RemoveHeadPayloadChunks();
            return;
        }

        // Per-player upload rate limit (sliding 1s window over accepted compressed bytes).
        if (const auto BudgetPerSecond = UCk_Utils_RenderTarget_Settings_UE::Get_ClientUploadMaxBytesPerSecond();
            BudgetPerSecond > 0)
        {
            auto PayloadBytes = 0;
            for (const auto& ChunkBytes : HeadPayloadChunks)
            { PayloadBytes += ChunkBytes.Num(); }

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InRenderTargetEntity);
            const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
            const auto NowSeconds = UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time().Get_Seconds();

            auto& Budget = InAssembly._Budgets.FindOrAdd(HeadSender);

            if (NowSeconds - Budget._WindowStartSeconds > 1.0f)
            {
                Budget._WindowStartSeconds = NowSeconds;
                Budget._BytesThisWindow = 0;
            }

            if (Budget._BytesThisWindow + PayloadBytes > BudgetPerSecond)
            {
                render_target::Warning(
                    TEXT("RenderTarget [{}] dropped a [{}] byte upload from [{}] — exceeds the [{}] bytes/sec budget"),
                    InRenderTargetEntity, PayloadBytes, HeadSender.Get(), BudgetPerSecond);
                RemoveHeadPayloadChunks();
                return;
            }

            Budget._BytesThisWindow += PayloadBytes;
        }

        InAssembly._ApplyJob = render_target::pixel::Launch_DecompressAndApplyJob(
            HeadChunk.Get_Kind(),
            render_target::pixel::Reassemble_Chunks(HeadPayloadChunks),
            HeadChunk.Get_UncompressedSize(),
            HeadChunk.Get_Size(),
            TArray<uint8>{}, FIntPoint::ZeroValue,
            UCk_Utils_RenderTarget_Settings_UE::Get_BlockSize());
        InAssembly._ApplyJobSender = HeadSender;

        RemoveHeadPayloadChunks();
    }

    auto
        FProcessor_RenderTarget_ReceiveClientUploads::
        DoFinishUploadApply(
            HandleType InRenderTargetEntity,
            const FFragment_RenderTarget_Current& InCurrent,
            FFragment_RenderTarget_PixelSync& InPixelSync,
            FFragment_RenderTarget_UploadAssembly& InAssembly)
        -> void
    {
        const auto JobResult = InAssembly._ApplyJob;
        const auto Sender = InAssembly._ApplyJobSender;
        InAssembly._ApplyJob.Reset();
        InAssembly._ApplyJobSender = nullptr;

        if (JobResult->_Failed)
        {
            render_target::Warning(TEXT("RenderTarget [{}] upload from [{}] failed to decompress — dropped"),
                InRenderTargetEntity, Sender.Get());
            return;
        }

        // The upload becomes the new authoritative pixel state: snapshot + local target. The
        // upload's watermark is irrelevant server-side — the server's own applied-batch seq is
        // the truth for what's baked into ITS pixels.
        InPixelSync._LastSyncedSnapshot = JobResult->_NewStaging;
        InPixelSync._SnapshotSize = JobResult->_Size;
        InPixelSync._SnapshotInstructionWatermark = InCurrent.Get_NextBatchSeq() - 1;

        ck_render_target_processor::DrawPixelsToTarget(
            InRenderTargetEntity, InCurrent, InPixelSync._LastSyncedSnapshot, JobResult->_Size, InAssembly._UploadTexture);

        const auto PayloadSeq = InPixelSync._NextPayloadSeq;
        InPixelSync._NextPayloadSeq = PayloadSeq + 1;

        // Re-broadcast to every OTHER client (the uploader's stream is skipped + promoted by
        // dispatch), and bump the channel's _PixelEpoch so receivers know a non-instruction
        // pixel change happened.
        auto& Payload = InRenderTargetEntity.AddOrGet<FFragment_RenderTarget_PendingPixelPayload>();
        Payload._Kind = ECk_RenderTarget_PixelPayloadKind::FullSync;
        Payload._PayloadSeq = PayloadSeq;
        Payload._Size = JobResult->_Size;
        Payload._UncompressedSize = JobResult->_NewStaging.Num();
        Payload._Bytes = render_target::pixel::Compress(JobResult->_NewStaging);
        Payload._InstructionWatermark = InPixelSync._SnapshotInstructionWatermark;
        Payload._ExcludePlayer = Sender;

        auto OwnerEntity = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InRenderTargetEntity);
        const auto SyncName = UCk_Utils_RenderTarget_UE::Get_SyncName(InRenderTargetEntity);

        if (ck::IsValid(OwnerEntity))
        {
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_RenderTarget>(
                OwnerEntity,
                [&](FCk_RepData_RenderTarget& RepData) -> void
                {
                    if (auto* Channel = RepData.Find_Channel(SyncName))
                    { Channel->Set_PixelEpoch(Channel->Get_PixelEpoch() + 1); }
                });
        }

        UUtils_Signal_RenderTarget_OnClientUploadApplied::Broadcast(InRenderTargetEntity,
            MakePayload(InRenderTargetEntity, Sender));

        render_target::Verbose(TEXT("RenderTarget [{}] applied a [{}x{}] upload from [{}] as payload seq [{}]"),
            InRenderTargetEntity, JobResult->_Size.X, JobResult->_Size.Y, Sender.Get(), PayloadSeq);
    }
}

// --------------------------------------------------------------------------------------------------------------------
