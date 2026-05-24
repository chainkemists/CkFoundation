#include "CkStateMachine_Replication.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

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

        const auto& Params = Entity.Get<ck::FFragment_Sm_Params>();
        if (Params.Get_AuthorityModel() != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
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
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_WithHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return; }

                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        Sm_EnqueueOrStash(Entity, NewPayload.Get_History());

                        ck::sm::VeryVerbose(TEXT("WithHistory OnChange for [{}] — history size [{}]"),
                            Entity, NewPayload.Get_History().Num());
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return; }

                        // First receipt — if Setup hasn't run yet (no FFragment_Sm_Current), the
                        // stash helper holds the entries for FlushPendingReplication_Drain to
                        // release once Setup lands.
                        const auto& Payload = Data.Get<FCk_RepData_StateMachine_WithHistory>();
                        Sm_EnqueueOrStash(Entity, Payload.Get_History());

                        ck::sm::VeryVerbose(TEXT("WithHistory OnAdd for [{}] — initial history size [{}]"),
                            Entity, Payload.Get_History().Num());
                    }
                });

            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_NoHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return; }

                        // WithoutHistory replicates the latest state only — synthesize a single
                        // transition event from local current → replicated current and route it
                        // through the stash-or-queue helper.
                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        const auto NewSeq = NewPayload.Get_Seq();

                        const auto LocalCurrentClass = Entity.Has<ck::FFragment_Sm_Current>()
                            ? Entity.Get<ck::FFragment_Sm_Current>().Get_CurrentStateClass()
                            : TSubclassOf<UCk_SmState_EntityScript>{};

                        const auto Event = FCk_Sm_TransitionEvent
                        {
                            LocalCurrentClass,
                            NewPayload.Get_CurrentStateClass(),
                            NewSeq,
                            NewPayload.Get_CurrentStateFingerprint()
                        };

                        Sm_EnqueueOrStash(Entity, TArray<FCk_Sm_TransitionEvent>{Event});

                        ck::sm::VeryVerbose(TEXT("NoHistory OnChange for [{}] — synthesized event for seq [{}]"),
                            Entity, NewSeq);
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return; }

                        // First receipt — only enqueue if Seq > 0 (a Seq of 0 indicates the SM
                        // hasn't transitioned yet on authority and there's nothing to snap to).
                        const auto& Payload = Data.Get<FCk_RepData_StateMachine_NoHistory>();
                        if (Payload.Get_Seq() <= 0)
                        { return; }

                        const auto Event = FCk_Sm_TransitionEvent
                        {
                            TSubclassOf<UCk_SmState_EntityScript>{},
                            Payload.Get_CurrentStateClass(),
                            Payload.Get_Seq(),
                            Payload.Get_CurrentStateFingerprint()
                        };

                        Sm_EnqueueOrStash(Entity, TArray<FCk_Sm_TransitionEvent>{Event});

                        ck::sm::VeryVerbose(TEXT("NoHistory OnAdd for [{}] — initial snap to state [{}]"),
                            Entity, Payload.Get_CurrentStateClass());
                    }
                });
        }
    };

    // Unity-build safety: prefix the static instance with the module so a stray anonymous-namespace
    // global of the same name in another .cpp can't collide (per project memory feedback).
    static FCk_StateMachineRepHandlerRegistrar GCkStateMachineRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
