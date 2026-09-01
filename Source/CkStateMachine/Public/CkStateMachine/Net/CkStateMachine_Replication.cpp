#include "CkStateMachine_Replication.h"

#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

// --------------------------------------------------------------------------------------------------------------------

namespace ck_state_machine_replication
{
    auto
    Sm_ShouldEchoSuppress(
        const FCk_Handle& Entity) -> bool
    {
        if (NOT Entity.Has<ck::FFragment_Sm_Params>())
        { return false; }

        const auto SmHandle = UCk_Utils_StateMachine_UE::CastChecked(Entity);
        if (UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(SmHandle) != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        { return false; }

        return UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(Entity)
            == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled;
    }

    // True pre-Setup (no FFragment_Sm_Current yet) AND while a stash is already in-flight — the
    // second clause is what preserves arrival order across back-to-back deliveries.
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

        ck::statemachine::MirrorRunStatus_OrDeferWhileReplaying(Entity, PayloadStatus);
    }

    // Returns the events the caller should hand to Sm_EnqueueOrStash; an EMPTY array means
    // "nothing to enqueue — the watermark was already advanced for you".
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
            // Advance the watermark anyway, or the next OnChange re-detects the same gap.
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

    // Dedups against InTarget's contents as well as the applied watermark: the queue drains one
    // event per tick, so a ring re-delivery mid-drain re-carries every still-queued event. Events
    // within one delivery are seq-ascending per target, so one up-front threshold covers the batch.
    auto
    Sm_AppendNewerEvents(
        TArray<FCk_Sm_TransitionEvent>& InTarget,
        const TArray<FCk_Sm_TransitionEvent>& InEvents,
        int32 InLastAppliedSeq) -> void
    {
        const auto MinAcceptSeq = InTarget.IsEmpty()
            ? InLastAppliedSeq
            : FMath::Max(InLastAppliedSeq, InTarget.Last().Get_Seq());

        for (const auto& Event : InEvents)
        {
            if (Event.Get_Seq() > MinAcceptSeq)
            { InTarget.Add(Event); }
        }
    }

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
            Sm_AppendNewerEvents(Stash, Events, LastApplied);
            return;
        }

        auto& Queue = Entity.AddOrGet<ck::FFragment_Sm_ReplayQueue>().Get_Queue();
        Sm_AppendNewerEvents(Queue, Events, LastApplied);
    }

    // Leg-2 receive dispatch. The root's container carries a MIX of root-level events (empty
    // _SubSmIdentity) and sub-SM events relayed through the root, each in its OWN seq space.
    // Partitioning per identity is load-bearing: a shared seq space corrupts the dedup watermark and
    // lets lagged-out recovery snap the root to a sub-SM's state class.
    auto
    Sm_DispatchWithHistory(
        FCk_Handle& RootEntity,
        const TArray<FCk_Sm_TransitionEvent>& InHistory) -> void
    {
        struct FSubSmBucket
        {
            TArray<FGameplayTag>           Identity;
            TArray<FCk_Sm_TransitionEvent> Events;
        };

        auto RootEvents = TArray<FCk_Sm_TransitionEvent>{};
        auto SubBuckets = TArray<FSubSmBucket>{};

        for (const auto& Event : InHistory)
        {
            const auto& Identity = Event.Get_SubSmIdentity();
            if (Identity.IsEmpty())
            {
                RootEvents.Add(Event);
                continue;
            }

            if (auto* Bucket = SubBuckets.FindByPredicate(
                    [&](const FSubSmBucket& InBucket) -> bool { return InBucket.Identity == Identity; }))
            {
                Bucket->Events.Add(Event);
            }
            else
            {
                SubBuckets.Add(FSubSmBucket{Identity, TArray<FCk_Sm_TransitionEvent>{Event}});
            }
        }

        if (NOT RootEvents.IsEmpty())
        {
            const auto ToApply = Sm_DetectAndHandleLaggedOut_WithHistory(RootEntity, RootEvents);
            Sm_EnqueueOrStash(RootEntity, ToApply);
        }

        if (SubBuckets.IsEmpty())
        { return; }

        const auto RootHandle = UCk_Utils_StateMachine_UE::CastChecked(RootEntity);

        for (auto& Bucket : SubBuckets)
        {
            auto TargetSubSm = UCk_Utils_StateMachine_UE::TryFind_ActiveSubSm_ByParentHierarchy(RootHandle, Bucket.Identity);
            if (ck::Is_NOT_Valid(TargetSubSm))
            {
                // The hosting parent state isn't active on this machine. A true parent-then-child race
                // (stash-and-defer) is NOT handled — log + drop beats landing on the wrong SM.
                ck::sm::Warning(TEXT("WithHistory Apply: sub-SM for identity-path (depth [{}]) not active under root [{}]; dropping [{}] relayed event(s)"),
                    Bucket.Identity.Num(), RootEntity, Bucket.Events.Num());
                continue;
            }

            auto TargetEntity = FCk_Handle{TargetSubSm};
            const auto ToApply = Sm_DetectAndHandleLaggedOut_WithHistory(TargetEntity, Bucket.Events);
            Sm_EnqueueOrStash(TargetEntity, ToApply);
        }
    }

    // Produce contract: READ-ONLY, never mutates the entity.
    auto
    Sm_CaptureStateOverrides(
        const FCk_Handle& Entity) -> TArray<FCk_Sm_SavedStateOverride>
    {
        auto Out = TArray<FCk_Sm_SavedStateOverride>{};
        if (NOT Entity.Has<ck::FFragment_Sm_StateOverrides>())
        { return Out; }

        for (const auto& Entry : Entity.Get<ck::FFragment_Sm_StateOverrides>().Get_Overrides())
        { Out.Add(FCk_Sm_SavedStateOverride{Entry._OverrideStateClass, Entry._CachedStatesToOverride}); }

        return Out;
    }

    // Re-installs saved runtime overrides through the SAME deferred Request_AddOverrideState the live
    // path uses. Construct rebuilds the SM but never re-adds runtime overrides, so this is the only
    // thing that puts them back. Why the deferred add provably lands before the resume transition:
    // CkStateMachine/CLAUDE.md § Save/load ordering.
    auto
    Sm_ReinstallSavedOverrides(
        FCk_Handle&                              Entity,
        const TArray<FCk_Sm_SavedStateOverride>& SavedOverrides) -> void
    {
        if (SavedOverrides.IsEmpty())
        { return; }

        auto SmHandle = UCk_Utils_StateMachine_UE::CastChecked(Entity);
        for (const auto& Saved : SavedOverrides)
        {
            if (ck::Is_NOT_Valid(Saved.Get_OverrideStateClass()))
            { continue; }

            UCk_Utils_StateMachine_UE::Request_AddOverrideState(SmHandle, Saved.Get_OverrideStateClass(), {});
        }
    }

    // Save-load hydration: drive the SM to its {RunStatus, CurrentStateClass} WITHOUT replaying entry
    // effects inline — stash the decision for FProcessor_Sm_HydrationResume, which steers the
    // freshly-composed SM there through its own Start/Transition ladder. NotReady-before-any-mutation:
    // the Current guard MUST precede the single-shot override re-add or a retry stacks duplicates.
    auto
    Sm_StashHydrationResume(
        FCk_Handle&                              Entity,
        ECk_SmRunStatus                          DesiredRunStatus,
        TSubclassOf<UCk_SmState_EntityScript>    DesiredStateClass,
        const TArray<FCk_Sm_SavedStateOverride>& SavedOverrides) -> ECk_Persistence_ApplyResult
    {
        if (NOT Entity.Has<ck::FFragment_Sm_Current>())
        { return ECk_Persistence_ApplyResult::NotReady; }

        // Overrides FIRST — the override write must precede the restore transition.
        Sm_ReinstallSavedOverrides(Entity, SavedOverrides);

        Entity.AddOrGet<ck::FFragment_Sm_HydrationResume>().Populate(DesiredRunStatus, DesiredStateClass);
        return ECk_Persistence_ApplyResult::Applied;
    }

    struct FCk_StateMachineRepHandlerRegistrar
    {
        FCk_StateMachineRepHandlerRegistrar()
        {
            // Both NetApply shapes always return Applied: pre-Setup arrivals stash via Sm_ShouldStash,
            // never via dispatcher NotReady retries, preserving FlushPendingReplication_Drain's order.

            FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_StateMachine_WithHistory>({
                    .Posture = ECk_Snapshot_Posture::Durable,
                    // Canonical single event {null -> CurrentStateClass, Seq 0, Fp 0}: live server seqs
                    // restart in the rebuilt world, so persisting the live ring would diverge. Gated on
                    // the replication MODEL (not _Replication) so DoesNotReplicate SMs persist too.
                    .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                    {
                        // State/task/condition/sub-machine graph entities are reconstruction-owned. They deliberately
                        // carry runtime SM fragments, but persisting those payloads would conflict with owner redrive.
                        if (Entity.Has<ck::FTag_Snapshot_SaveTransient>())
                        { return {}; }
                        if (NOT Entity.Has<ck::FFragment_Sm_Current>() || NOT Entity.Has<ck::FFragment_Sm_Params>())
                        { return {}; }
                        const auto& Params = Entity.Get<ck::FFragment_Sm_Params>();
                        if (NOT Params.Get_ShouldPersistCurrentState()
                            || Params.Get_ReplicationModel() != ECk_Sm_ReplicationModel::WithHistory)
                        { return {}; }

                        const auto& Current = Entity.Get<ck::FFragment_Sm_Current>();

                        auto Payload = FCk_RepData_StateMachine_WithHistory{};
                        Payload.Set_RunStatus(Current.Get_RunStatus());
                        if (ck::IsValid(Current.Get_CurrentStateClass()))
                        {
                            Payload.Get_History().Add(FCk_Sm_TransitionEvent{
                                TSubclassOf<UCk_SmState_EntityScript>{}, Current.Get_CurrentStateClass(), 0, 0});
                        }
                        Payload.Set_SavedStateOverrides(Sm_CaptureStateOverrides(Entity));
                        return FInstancedStruct::Make(Payload);
                    },
                    .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return ECk_Persistence_ApplyResult::Applied; }

                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        Sm_DispatchWithHistory(Entity, NewPayload.Get_History());
                        Sm_HandleRunStatus(Entity, NewPayload.Get_RunStatus());

                        ck::sm::VeryVerbose(TEXT("WithHistory Apply for [{}] — history size [{}], status [{}]"),
                            Entity, NewPayload.Get_History().Num(), NewPayload.Get_RunStatus());

                        return ECk_Persistence_ApplyResult::Applied;
                    },
                    .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        if (Entity.Has<ck::FFragment_Sm_Params>()
                            && NOT Entity.Get<ck::FFragment_Sm_Params>().Get_ShouldPersistCurrentState())
                        { return ECk_Persistence_ApplyResult::Applied; }

                        const auto& Payload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        const auto DesiredStateClass = Payload.Get_History().IsEmpty()
                            ? TSubclassOf<UCk_SmState_EntityScript>{}
                            : Payload.Get_History().Last().Get_NewStateClass();
                        return Sm_StashHydrationResume(Entity, Payload.Get_RunStatus(), DesiredStateClass,
                            Payload.Get_SavedStateOverrides());
                    }});

            FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_StateMachine_NoHistory>({
                    .Posture = ECk_Snapshot_Posture::Durable,
                    // Latest state only, canonical Seq 0 / Fp 0. Gated on the replication MODEL (not
                    // _Replication) so DoesNotReplicate WithoutHistory SMs persist too.
                    .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                    {
                        if (Entity.Has<ck::FTag_Snapshot_SaveTransient>())
                        { return {}; }
                        if (NOT Entity.Has<ck::FFragment_Sm_Current>() || NOT Entity.Has<ck::FFragment_Sm_Params>())
                        { return {}; }
                        const auto& Params = Entity.Get<ck::FFragment_Sm_Params>();
                        if (NOT Params.Get_ShouldPersistCurrentState()
                            || Params.Get_ReplicationModel() != ECk_Sm_ReplicationModel::WithoutHistory)
                        { return {}; }

                        const auto& Current = Entity.Get<ck::FFragment_Sm_Current>();

                        auto Payload = FCk_RepData_StateMachine_NoHistory{};
                        Payload.Set_CurrentStateClass(Current.Get_CurrentStateClass());
                        Payload.Set_RunStatus(Current.Get_RunStatus());
                        Payload.Set_SavedStateOverrides(Sm_CaptureStateOverrides(Entity));
                        return FInstancedStruct::Make(Payload);
                    },
                    .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                    {
                        if (Sm_ShouldEchoSuppress(Entity))
                        { return ECk_Persistence_ApplyResult::Applied; }

                        // WithoutHistory replicates the latest state only — synthesize one transition
                        // event from local current to replicated current. Old unset plus Seq 0 means
                        // authority hasn't transitioned yet: nothing to snap to, run status still mirrors.
                        const auto& NewPayload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        const auto NewSeq = NewPayload.Get_Seq();

                        if (Old.IsSet() || NewSeq > 0)
                        {
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

                        return ECk_Persistence_ApplyResult::Applied;
                    },
                    .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        if (Entity.Has<ck::FFragment_Sm_Params>()
                            && NOT Entity.Get<ck::FFragment_Sm_Params>().Get_ShouldPersistCurrentState())
                        { return ECk_Persistence_ApplyResult::Applied; }

                        const auto& Payload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        return Sm_StashHydrationResume(Entity, Payload.Get_RunStatus(), Payload.Get_CurrentStateClass(),
                            Payload.Get_SavedStateOverrides());
                    }});
        }
    };

    // Unity-build safety: module-prefixed so a same-named global in another .cpp can't collide.
    static FCk_StateMachineRepHandlerRegistrar GCkStateMachineRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
