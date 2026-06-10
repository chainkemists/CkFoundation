#include "CkRenderTarget_Replication.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Replication-handler registration for the RenderTarget instruction channel (channel A).
//
// The container fragment lives on the OWNER entity (the one with the replication driver) and
// carries one ChannelState per RenderTarget sync child. The handler resolves each channel's sync
// child by sync name (NotReady until the symmetric client-side composition has created them all),
// then routes new batches to either the child's stash (Setup not done / stash already in flight —
// CkStateMachine spec §5.4 precedence) or its replay queue.
// FProcessor_RenderTarget_ApplyReplicatedBatches commits.
//
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // NOTE on echo suppression: a client that authored a batch applied it locally at request
    // time, so the server's republication must not apply it a second time there. That filter
    // lives in FProcessor_RenderTarget_ApplyReplicatedBatches as skip-but-advance-watermark —
    // NOT here — because with multiple authoring clients a handler-side filter would leave seq
    // holes at a client's own batches and falsely trip the ring-wrap gap recovery.

    auto
    RenderTarget_ShouldStash(
        const FCk_Handle_RenderTarget& InSyncEntity) -> bool
    {
        if (InSyncEntity.Has<ck::FTag_RenderTarget_NeedsSetup>())
        { return true; }

        if (InSyncEntity.Has<ck::FFragment_RenderTarget_PendingReplicationBatches>())
        {
            const auto& Stash = InSyncEntity.Get<ck::FFragment_RenderTarget_PendingReplicationBatches>();
            if (NOT Stash.Get_Stash().IsEmpty())
            { return true; }
        }

        return false;
    }

    auto
    RenderTarget_EnqueueOrStash(
        const FCk_Handle& InOwnerEntity,
        FCk_Handle_RenderTarget& InSyncEntity,
        const FCk_RenderTarget_ChannelState& InChannel) -> void
    {
        const auto LastApplied = InSyncEntity.Has<ck::FFragment_RenderTarget_ClientReplayState>()
            ? InSyncEntity.Get<ck::FFragment_RenderTarget_ClientReplayState>().Get_LastAppliedSeq()
            : 0;

        auto NewBatches = TArray<FCk_RenderTarget_InstructionBatch>{};
        for (const auto& Batch : InChannel.Get_Batches())
        {
            if (Batch.Get_Seq() <= LastApplied)
            { continue; }

            NewBatches.Add(Batch);
        }

        if (NewBatches.IsEmpty())
        { return; }

        if (RenderTarget_ShouldStash(InSyncEntity))
        {
            auto& Stash = InSyncEntity.AddOrGet<ck::FFragment_RenderTarget_PendingReplicationBatches>().Get_Stash();
            Stash.Append(NewBatches);
            return;
        }

        auto& Queue = InSyncEntity.AddOrGet<ck::FFragment_RenderTarget_ReplayQueue>().Get_Queue();
        Queue.Append(NewBatches);
    }

    struct FCk_RenderTargetRepHandlerRegistrar
    {
        FCk_RenderTargetRepHandlerRegistrar()
        {
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_RenderTarget::StaticStruct(); },
                {
                    .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                    {
                        const auto& Payload = New.Get<FCk_RepData_RenderTarget>();

                        // All channels must resolve before ANY applies — otherwise a partial apply
                        // would advance some channels and starve the unresolved ones on the retry
                        // (the dispatcher retries the whole entry).
                        for (const auto& Channel : Payload.Get_Channels())
                        {
                            if (ck::Is_NOT_Valid(UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(Entity, Channel.Get_SyncName())))
                            { return ECk_RepFragment_ApplyResult::NotReady; }
                        }

                        for (const auto& Channel : Payload.Get_Channels())
                        {
                            auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(Entity, Channel.Get_SyncName());
                            RenderTarget_EnqueueOrStash(Entity, SyncEntity, Channel);
                        }

                        ck::render_target::VeryVerbose(TEXT("RepData apply on [{}] — [{}] channel(s)"),
                            Entity, Payload.Get_Channels().Num());

                        return ECk_RepFragment_ApplyResult::Applied;
                    }
                });
        }
    };

    // Unity-build safety: module-prefixed name so a same-named static in another .cpp can't collide.
    static FCk_RenderTargetRepHandlerRegistrar GCkRenderTargetRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
