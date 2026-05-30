#pragma once

#include "CkStateMachine_Fragment.h"

#include "CkStateMachine/Condition/CkSmCondition_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // COMMIT PENDING SCRIPT ATTACH — Materializes deferred EntityScripts on SM child entities
    // before the EntityScript construction pipeline observes them. Runs in the same script
    // group as FProcessor_EntityScript_ContinueConstruction and must precede it so the
    // attach's FTag_EntityScript_ContinueConstruction is in place for the same frame.
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_SmScript_CommitPendingAttach : public ck_exp::TProcessor<
        FProcessor_SmScript_CommitPendingAttach,
        FCk_Handle,
        ck::TReadOnly<FFragment_SmScript_PendingAttach>,
        FTag_SmScript_PendingAttach,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_Script;
        using RunBefore     = TDepList<FProcessor_EntityScript_ContinueConstruction>;
        using MarkedDirtyBy = FTag_SmScript_PendingAttach;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmScript_PendingAttach& InPending) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ================================================================================================================
    // SETUP — One-time initialization, auto-start if configured
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_Setup : public ck_exp::TProcessor<
        FProcessor_Sm_Setup,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        FTag_Sm_RequiresSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using MarkedDirtyBy = FTag_Sm_RequiresSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent) -> void;
    };

    // ================================================================================================================
    // HANDLE REQUESTS — Process Start/Stop/Pause/Resume/Transition
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Sm_HandleRequests,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadOnly<FFragment_Sm_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Sm_Setup>;
        using MarkedDirtyBy = FFragment_Sm_Requests;

        friend class FProcessor_Sm_CommitPendingTransition;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FFragment_Sm_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_AddOverrideState& InRequest) -> void;

    private:
        static auto
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> void;

        // InScheduleDestroy=false runs the exit cascade but leaves the previous state entity
        // alive; the caller (transition path) is then responsible for destroying it after the
        // new state is committed. See FProcessor_Sm_CommitPendingTransition.
        static auto
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            bool InScheduleDestroy = true) -> void;
    };

    // Forward decl — the processors below name CommitPendingTransition in their RunBefore, but
    // declaring it first keeps the replay path next to the request path it complements.
    class FProcessor_Sm_CommitPendingTransition;
    class FProcessor_Sm_ApplyReplicatedHistory;

    // ================================================================================================================
    // FLUSH PENDING REPLICATION — DRAIN — Releases stashed rep entries into the replay queue.
    //
    // The OnChange/OnAdd handlers (CkStateMachine_Replication.cpp) park incoming events in
    // FFragment_Sm_PendingReplicationEntries when either FFragment_Sm_Current is not yet present
    // (Setup hasn't run on the client) or the stash already has entries (preserving arrival
    // order under back-to-back deliveries). This processor releases the stash into ReplayQueue
    // in arrival order once Setup is complete and the SM is not faulted.
    //
    // Runs BEFORE ApplyReplicatedHistory in the same group so a newly-released entry can be
    // drained from the queue in the same tick.
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_FlushPendingReplication_Drain : public ck_exp::TProcessor<
        FProcessor_Sm_FlushPendingReplication_Drain,
        FCk_Handle_StateMachine,
        TReadWrite<FFragment_Sm_Current>,
        TReadWrite<FFragment_Sm_PendingReplicationEntries>,
        TExclude<FTag_Sm_DeterminismFault>,
        TExclude<FTag_Sm_RequiresSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        using RunBefore     = TDepList<FProcessor_Sm_ApplyReplicatedHistory>;
        using MarkedDirtyBy = FFragment_Sm_PendingReplicationEntries;

    public:
        using TProcessor::TProcessor;

    public:
        // Note: FFragment_Sm_Current is TReadWrite because MirrorRunStatus mutates it when
        // applying the stashed run-status after events drain.
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_PendingReplicationEntries& InStash) -> void;
    };

    // ================================================================================================================
    // APPLY REPLICATED HISTORY — Non-owning client replay path. Drains FFragment_Sm_ReplayQueue
    // one entry per tick into FFragment_Sm_PendingTransition, which the CommitPendingTransition
    // processor then lands as a real state transition. The replicated history populates the queue
    // via the OnChange handler registered in CkStateMachine_Replication.cpp.
    //
    // TExclude<FFragment_Sm_PendingTransition>: don't double-stack a new replay entry on top of
    // an in-flight transition — wait for it to commit first.
    // TExclude<FTag_Sm_DeterminismFault>: fingerprint mismatch puts the SM in a faulted state
    // where no further transitions land until the fault is cleared (spec §9).
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_ApplyReplicatedHistory : public ck_exp::TProcessor<
        FProcessor_Sm_ApplyReplicatedHistory,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadWrite<FFragment_Sm_ReplayQueue>,
        TExclude<FFragment_Sm_PendingTransition>,
        TExclude<FTag_Sm_DeterminismFault>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        using RunAfter      = TDepList<FProcessor_Sm_HandleRequests>;
        using RunBefore     = TDepList<FProcessor_Sm_CommitPendingTransition>;
        using MarkedDirtyBy = FFragment_Sm_ReplayQueue;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_ReplayQueue& InQueue) -> void;
    };

    // ================================================================================================================
    // COMMIT PENDING TRANSITION — Defer the new state's entry until the previous state's
    // exit cascade has fully drained.
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_CommitPendingTransition : public ck_exp::TProcessor<
        FProcessor_Sm_CommitPendingTransition,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadWrite<FFragment_Sm_PendingTransition>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        using RunAfter      = TDepList<FProcessor_SmCondition_Exit>;
        using MarkedDirtyBy = FFragment_Sm_PendingTransition;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_PendingTransition& InPending) -> void;
    };

    // ================================================================================================================
    // PUSH OWNING-CLIENT BATCH — End-of-frame flush of locally-buffered transitions to the server.
    //
    // The owning client of an OwningClientAuthoritative SM applies transitions locally for zero
    // latency (CommitPendingTransition writes them into FFragment_Sm_PendingClientBatch instead
    // of the replicated payload). This processor drains the batch into a single RPC call on the
    // per-actor StateMachineRelay (Server_PushTransitionBatch / Server_PushCurrentState). The
    // server's RPC handler — wired in a follow-up Phase 10 task — resolves the SM and reconstructs
    // the transitions, after which the standard server-side publication path takes over.
    //
    // ClientOnly NetModeRequirement: the processor is created only on machines that have
    // FTag_NetMode_IsClient (pure clients + listen-server clients excluded). Server processes
    // never push to themselves.
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_PushOwningClientBatch : public ck_exp::TProcessor<
        FProcessor_Sm_PushOwningClientBatch,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_PendingClientBatch>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Replication;
        using MarkedDirtyBy = FFragment_Sm_PendingClientBatch;

        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_PendingClientBatch& InBatch) -> void;
    };

    // ================================================================================================================
    // ENDPLAY — Cleanup on SM entity destruction
    // ================================================================================================================

    class CKSTATEMACHINE_API FProcessor_Sm_EndPlay : public ck_exp::TProcessor<
        FProcessor_Sm_EndPlay,
        FCk_Handle_StateMachine,
        TReadWrite<FFragment_Sm_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
