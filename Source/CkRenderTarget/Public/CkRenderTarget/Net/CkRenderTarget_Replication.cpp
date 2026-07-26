#include "CkRenderTarget_Replication.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/Net/CkRenderTarget_RepData.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Fragment.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Processor.h" // FProcessor_RenderTarget_HandleRequests::HydrateFromSavedChannel
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_render_target_replication
{
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
            FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_RenderTarget>({
                    // Deliberately not gated on Replicates — CkRenderTarget/Claude.md.
                    .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                    {
                        if (NOT Entity.Has<ck::FFragment_RenderTarget_Params>()
                            || NOT Entity.Has<FFragment_RenderTarget_AuthoredLog>())
                        { return {}; }

                        const auto& Params      = Entity.Get<ck::FFragment_RenderTarget_Params>();
                        const auto& AuthoredLog = Entity.Get<FFragment_RenderTarget_AuthoredLog>();
                        const auto& Batches     = AuthoredLog.Get_Batches();

                        auto Channel = FCk_RenderTarget_ChannelState{}
                            .Set_SyncName(Params.Get_SyncName())
                            .Set_Batches(Batches);

                        if (NOT Batches.IsEmpty())
                        { Channel.Set_LatestSeq(Batches.Last().Get_Seq()); }

                        auto RepData = FCk_RepData_RenderTarget{};
                        RepData.Get_Channels().Emplace(MoveTemp(Channel));
                        return FInstancedStruct::Make(RepData);
                    },
                    .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        const auto& Payload = New.Get<FCk_RepData_RenderTarget>();

                        // All channels must resolve before ANY applies: the dispatcher retries the
                        // whole entry, so a partial apply would starve the unresolved channels.
                        for (const auto& Channel : Payload.Get_Channels())
                        {
                            if (ck::Is_NOT_Valid(UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(Entity, Channel.Get_SyncName())))
                            { return ECk_Persistence_ApplyResult::NotReady; }
                        }

                        for (const auto& Channel : Payload.Get_Channels())
                        {
                            auto SyncEntity = UCk_Utils_RenderTarget_UE::TryGet_RenderTarget(Entity, Channel.Get_SyncName());
                            RenderTarget_EnqueueOrStash(Entity, SyncEntity, Channel);
                        }

                        ck::render_target::VeryVerbose(TEXT("RepData apply on [{}] — [{}] channel(s)"),
                            Entity, Payload.Get_Channels().Num());

                        return ECk_Persistence_ApplyResult::Applied;
                    },
                    // Authority-side, and Entity is the sync CHILD here (child-keyed payload) —
                    // not the owner NetApply resolves against. Why: CkRenderTarget/Claude.md.
                    .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        const auto& Payload  = New.Get<FCk_RepData_RenderTarget>();
                        const auto& Channels = Payload.Get_Channels();
                        if (Channels.IsEmpty())
                        { return ECk_Persistence_ApplyResult::Applied; }

                        return ck::FProcessor_RenderTarget_HandleRequests::HydrateFromSavedChannel(Entity, Channels[0])
                            ? ECk_Persistence_ApplyResult::Applied
                            : ECk_Persistence_ApplyResult::NotReady;
                    }});
        }
    };

    // Unity-build safety: module-prefixed name so a same-named static in another .cpp can't collide.
    static FCk_RenderTargetRepHandlerRegistrar GCkRenderTargetRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
