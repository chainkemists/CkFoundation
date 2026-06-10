#include "CkStateMachine_Replication.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Replication-handler registration for CkStateMachine's two payload shapes.
//
// Both RepData types are container-style replicated fragments (no per-entity UObject driver),
// attached on authority by FProcessor_Sm_Setup and replicated via FCk_RepData_Container. The
// handlers below are STUBS that log only — the actual replay-and-commit logic lives in Phase 7's
// ApplyReplicatedHistory processor. Registering the handlers now (Phase 5.1) means the rep
// driver doesn't silently drop changes on the client side once Phase 5.2 starts attaching the
// payload.
//
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Spec §5.5 echo suppression: the owning client of an OwningClientAuthoritative SM applies
    // transitions locally (zero-latency), batches them into FFragment_Sm_PendingClientBatch,
    // then pushes the batch over RPC. The server applies them and re-publishes via the rep
    // payload. That replicated payload makes a round trip back to the owning client as a
    // rep delta — the owning client must NOT route it through the replay queue, or the
    // transition would land twice (once locally at commit time, once again on rep arrival).
    //
    // Detection: the SM is OwningClientAuthoritative AND this machine is the actor's owning
    // player. Non-owning clients on the same SM continue to apply the rep payload normally.
    auto
    Sm_ShouldEchoSuppress(
        const FCk_Handle& Entity) -> bool
    {
        if (NOT Entity.Has<ck::FFragment_Sm_Params>())
        { return false; }

        const auto SmHandle = ck::StaticCast<FCk_Handle_StateMachine>(Entity);
        if (UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(SmHandle) != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        { return false; }

        return UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(Entity)
            == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;
    }

    // Spec §5.4 stash-precedence: rep payloads that arrive before the client's Setup processor
    // has run (no FFragment_Sm_Current yet) must NOT bypass that setup — they're held in
    // FFragment_Sm_PendingReplicationEntries until FlushPendingReplication_Drain releases them
    // in arrival order. Additionally, while a stash is in-flight, all new arrivals also stash
    // so the original ordering is preserved (otherwise a later OnChange could land ahead of
    // earlier stashed entries on the next pump).
    auto
    Sm_ShouldStash(
        const FCk_Handle& Entity) -> bool
    {
        if (NOT Entity.Has<ck::FFragment_Sm_Current>())
        { return true; }

        if (Entity.Has<ck::FFragment_Sm_PendingReplicationEntries>())
        {
            const auto& Stash = Entity.Get<ck::FFragment_Sm_PendingReplicationEntries>();
            if (NOT Stash.Get_StashedEntries().IsEmpty())
            { return true; }
        }

        return false;
    }

    // Phase 11 — receive-side handler for the RepData's run-status. Mirror immediately when the
    // client has FFragment_Sm_Current; otherwise stash the value alongside the events for
    // FlushPendingReplication_Drain to apply once Setup completes. Spec §5.4 ordering preserved.
    auto
    Sm_HandleRunStatus(
        FCk_Handle& Entity,
        ECk_SmRunStatus PayloadStatus) -> void
    {
        if (Sm_ShouldStash(Entity))
        {
            auto& Stash = Entity.AddOrGet<ck::FFragment_Sm_PendingReplicationEntries>();
            Stash.Set_PendingRunStatus(PayloadStatus);
            Stash.Set_HasPendingRunStatus(true);
            return;
        }

        ck::statemachine::MirrorRunStatus(Entity, PayloadStatus);
    }

    // Spec §5.7 lagged-out gap recovery (WithHistory).
    //
    // The replicated ring buffer holds the last RingSize events. If a client has been quiet long
    // enough for the authority to roll older events out of the ring, the client's LastApplied
    // watermark sits below the earliest seq the payload carries — there's no contiguous path
    // from LastApplied+1 to History[0].Seq. The recovery: discard the partial history, synthesize
    // a single snap-to-current event from local current → history.Last(), and let the replay
    // engine commit it as one transition.
    //
    // Snap-no-op: if local current already equals the target (e.g. the client missed a B→A loop
    // and is still in A), don't enqueue anything — just advance the watermark to avoid
    // re-triggering on the next delta. Returns the events the caller should hand to
    // Sm_EnqueueOrStash; empty array means "nothing to enqueue, watermark was advanced for you".
    auto
    Sm_DetectAndHandleLaggedOut_WithHistory(
        FCk_Handle& Entity,
        const TArray<FCk_Sm_TransitionEvent>& InHistory) -> TArray<FCk_Sm_TransitionEvent>
    {
        if (InHistory.IsEmpty())
        { return {}; }

        const auto LastApplied = Entity.Has<ck::FFragment_Sm_ClientReplayState>()
            ? Entity.Get<ck::FFragment_Sm_ClientReplayState>().Get_ClientLastAppliedSeq()
            : 0;

        const auto FirstSeq = InHistory[0].Get_Seq();
        const auto IsLaggedOut = FirstSeq > LastApplied + 1;
        if (NOT IsLaggedOut)
        { return InHistory; }

        const auto& Latest = InHistory.Last();
        ck::sm::Warning(
            TEXT("Lagged-out client recovery on [{}] — LastApplied=[{}], history range [{}..{}], snapping to [{}] (seq [{}])"),
            Entity, LastApplied, FirstSeq, Latest.Get_Seq(),
            Latest.Get_NewStateClass(), Latest.Get_Seq());

        const auto LocalCurrentClass = Entity.Has<ck::FFragment_Sm_Current>()
            ? Entity.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass()
            : TSubclassOf<UCk_SmState_EntityScript>{};

        if (LocalCurrentClass == Latest.Get_NewStateClass())
        {
            // Snap-no-op — already in the target state. Advance watermark and bail without
            // enqueuing anything; otherwise the next OnChange would re-detect the same gap.
            auto& ReplayState = Entity.AddOrGet<ck::FFragment_Sm_ClientReplayState>();
            ReplayState.Set_ClientLastAppliedSeq(Latest.Get_Seq());
            return {};
        }

        const auto SnapEvent = FCk_Sm_TransitionEvent
        {
            LocalCurrentClass,
            Latest.Get_NewStateClass(),
            Latest.Get_Seq(),
            Latest.Get_NewStateFingerprint()
        };

        return TArray<FCk_Sm_TransitionEvent>{SnapEvent};
    }

    // Append events to either the stash (if Sm_ShouldStash) or directly to the replay queue.
    // Filters by ClientLastAppliedSeq so duplicate or out-of-order arrivals are deduplicated.
    auto
    Sm_EnqueueOrStash(
        FCk_Handle& Entity,
        TArray<FCk_Sm_TransitionEvent> Events) -> void
    {
        if (Events.IsEmpty())
        { return; }

        const auto LastApplied = Entity.Has<ck::FFragment_Sm_ClientReplayState>()
            ? Entity.Get<ck::FFragment_Sm_ClientReplayState>().Get_ClientLastAppliedSeq()
            : 0;

        if (Sm_ShouldStash(Entity))
        {
            auto& Stash = Entity.AddOrGet<ck::FFragment_Sm_PendingReplicationEntries>().Get_StashedEntries();
            for (const auto& Event : Events)
            {
                if (Event.Get_Seq() > LastApplied)
                { Stash.Add(Event); }
            }
            return;
        }

        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
        for (const auto& Event : Events)
        {
            if (Event.Get_Seq() > LastApplied)
            { Queue.Add(Event); }
        }
    }

    struct FCk_StateMachineRepHandlerRegistrar
    {
        FCk_StateMachineRepHandlerRegistrar()
        {
            // Both shapes keep their own spec-governed receive machinery (§5.4 stash-and-flush,
            // §5.5 echo suppression, §5.7 lagged-out recovery) and therefore always return Applied
            // — pre-Setup arrivals stash via Sm_ShouldStash, never via dispatcher NotReady retries,
            // preserving the arrival-order contract FlushPendingReplication_Drain relies on.

            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_WithHistory::StaticStruct(); },
                {
                    .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return ECk_RepFragment_ApplyResult::Applied; }

                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        const auto EventsToApply = Sm_DetectAndHandleLaggedOut_WithHistory(Entity, NewPayload.Get_History());
                        Sm_EnqueueOrStash(Entity, EventsToApply);
                        Sm_HandleRunStatus(Entity, NewPayload.Get_RunStatus());

                        ck::sm::VeryVerbose(TEXT("WithHistory Apply for [{}] — history size [{}], status [{}]"),
                            Entity, NewPayload.Get_History().Num(), NewPayload.Get_RunStatus());

                        return ECk_RepFragment_ApplyResult::Applied;
                    }
                });

            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_NoHistory::StaticStruct(); },
                {
                    .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return ECk_RepFragment_ApplyResult::Applied; }

                        // WithoutHistory replicates the latest state only — synthesize a single
                        // transition event from local current → replicated current and route it
                        // through the stash-or-queue helper. On first application (Old unset) a
                        // Seq of 0 means the SM hasn't transitioned on authority yet — nothing to
                        // snap to; the run status still mirrors.
                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        const auto NewSeq = NewPayload.Get_Seq();

                        if (Old.IsSet() || NewSeq > 0)
                        {
                            // First application snaps from a null From-class (nothing applied yet);
                            // subsequent applications transition from the local current state.
                            const auto FromClass = Old.IsSet() && Entity.Has<ck::FFragment_Sm_Current>()
                                ? Entity.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass()
                                : TSubclassOf<UCk_SmState_EntityScript>{};

                            const auto Event = FCk_Sm_TransitionEvent
                            {
                                FromClass,
                                NewPayload.Get_CurrentStateClass(),
                                NewSeq,
                                NewPayload.Get_CurrentStateFingerprint()
                            };

                            Sm_EnqueueOrStash(Entity, TArray<FCk_Sm_TransitionEvent>{Event});
                        }

                        Sm_HandleRunStatus(Entity, NewPayload.Get_RunStatus());

                        ck::sm::VeryVerbose(TEXT("NoHistory Apply for [{}] — seq [{}], status [{}]"),
                            Entity, NewSeq, NewPayload.Get_RunStatus());

                        return ECk_RepFragment_ApplyResult::Applied;
                    }
                });
        }
    };

    // Unity-build safety: prefix the static instance with the module so a stray anonymous-namespace
    // global of the same name in another .cpp can't collide (per project memory feedback).
    static FCk_StateMachineRepHandlerRegistrar GCkStateMachineRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
