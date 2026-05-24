#include "CkStateMachine_Replication.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

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
    struct FCk_StateMachineRepHandlerRegistrar
    {
        FCk_StateMachineRepHandlerRegistrar()
        {
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_WithHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        // Append every replicated entry whose Seq is past our watermark. The
                        // ApplyReplicatedHistory processor drains the queue one entry per tick,
                        // bumping ClientLastAppliedSeq as each commits — so an OnChange delivering
                        // a contiguous window only enqueues entries strictly newer than what we
                        // last applied. Phase 8 will layer stash-and-flush on top of this for the
                        // OnAdd-vs-Setup race; Phase 10 will add echo suppression on owning client.
                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        const auto& History    = NewPayload.Get_History();

                        const auto LastApplied = Entity.Has<ck::FFragment_Sm_ClientReplayState>()
                            ? Entity.Get<ck::FFragment_Sm_ClientReplayState>().Get_ClientLastAppliedSeq()
                            : 0;

                        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
                        for (const auto& Event : History)
                        {
                            if (Event.Get_Seq() > LastApplied)
                            { Queue.Add(Event); }
                        }

                        ck::sm::VeryVerbose(TEXT("WithHistory OnChange for [{}] — history size [{}], queue size [{}], lastApplied [{}]"),
                            Entity, History.Num(), Queue.Num(), LastApplied);
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
                        // OnAdd fires when the client first receives the replicated fragment for
                        // an SM. For now we treat it the same as OnChange — append every entry —
                        // but Phase 8's stash-precedence check goes here: if Setup hasn't run yet
                        // on the client (no FFragment_Sm_Current), entries go to a stash that the
                        // FlushPendingReplication processors drain after Setup completes.
                        const auto& Payload = Data.Get<FCk_RepData_StateMachine_WithHistory>();
                        const auto& History = Payload.Get_History();

                        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
                        for (const auto& Event : History)
                        { Queue.Add(Event); }

                        ck::sm::VeryVerbose(TEXT("WithHistory OnAdd for [{}] — initial history size [{}]"),
                            Entity, History.Num());
                    }
                });

            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_NoHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        // WithoutHistory replicates the latest state only — no rolling buffer to
                        // walk. We synthesize a single transition event from local current →
                        // replicated current, and gate it on the seq advancing past our watermark
                        // so a duplicate OnChange doesn't double-apply.
                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        const auto NewSeq = NewPayload.Get_Seq();

                        const auto LastApplied = Entity.Has<ck::FFragment_Sm_ClientReplayState>()
                            ? Entity.Get<ck::FFragment_Sm_ClientReplayState>().Get_ClientLastAppliedSeq()
                            : 0;

                        if (NewSeq <= LastApplied)
                        { return; }

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

                        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
                        Queue.Add(Event);

                        ck::sm::VeryVerbose(TEXT("NoHistory OnChange for [{}] — synthesized event for seq [{}]"),
                            Entity, NewSeq);
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
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

                        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
                        Queue.Add(Event);

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
