#pragma once

#include "CkStateMachine_Fragment.h"

#include "CkStateMachine/Condition/CkSmCondition_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Processor.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------
    // Materializes deferred EntityScripts on SM child entities before the EntityScript construction
    // pipeline observes them; must precede FProcessor_EntityScript_ContinueConstruction so the
    // attach's FTag_EntityScript_ContinueConstruction is in place for the same frame.

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
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

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

    // --------------------------------------------------------------------------------------------------------------------

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
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

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

    // --------------------------------------------------------------------------------------------------------------------

    class CKSTATEMACHINE_API FProcessor_Sm_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Sm_HandleRequests,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadOnly<FFragment_Sm_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_AI;
        using RunAfter = TDepList<FProcessor_Sm_Setup>;
        using MarkedDirtyBy = FFragment_Sm_Requests;
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

        friend class FProcessor_Sm_CommitPendingTransition;
        friend class FProcessor_Sm_FirstSyncInitialState;

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
        // Every overload returns the outcome the drain reports to the request's completion delegate.
        // The run-status guards below are genuine rejections, not deferrals: a Start on a running SM
        // or a Transition on a non-Running one is dropped outright and never retried.
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Start& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Stop& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Pause& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Resume& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_Transition& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            const FCk_Request_Sm_AddOverrideState& InRequest) -> ECk_Request_OperationResult;

    private:
        static auto
        DoEnterState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> void;

        // InScheduleDestroy=false runs the exit cascade but leaves the previous state entity alive;
        // the transition path then destroys it after FProcessor_Sm_CommitPendingTransition commits.
        static auto
        DoExitCurrentState(
            HandleType InSmHandle,
            FFragment_Sm_Current& InCurrent,
            bool InScheduleDestroy = true) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed SM's still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKSTATEMACHINE_API FProcessor_Sm_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Sm_CancelPendingRequests,
        FCk_Handle_StateMachine,
        ck::TReadOnly<FFragment_Sm_Requests>,
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
            const FFragment_Sm_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_Sm_CommitPendingTransition;
    class FProcessor_Sm_ApplyReplicatedHistory;

    // --------------------------------------------------------------------------------------------------------------------
    // FIRST-SYNC INITIAL STATE — a non-authority machine never runs Start, and the initial entry is
    // not a replayed transition, so without this it sits at <none> until the first transition drains
    // (forever for a sink-state SM). Tagged by MirrorRunStatus; enters the locally-known initial
    // state and fires the initial OnSmStateChanged locally, with no publish.

    class CKSTATEMACHINE_API FProcessor_Sm_FirstSyncInitialState : public ck_exp::TProcessor<
        FProcessor_Sm_FirstSyncInitialState,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        FTag_Sm_NeedsInitialStateEntry,
        TExclude<FFragment_Sm_PendingTransition>,
        TExclude<FTag_Sm_DeterminismFault>,
        TExclude<FTag_Sm_RequiresSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay_AI;
        using RunAfter      = TDepList<FProcessor_Sm_FlushPendingReplication_Drain>;
        using RunBefore     = TDepList<FProcessor_Sm_ApplyReplicatedHistory>;
        using MarkedDirtyBy = FTag_Sm_NeedsInitialStateEntry;
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // FLUSH PENDING REPLICATION — DRAIN. Releases entries the OnChange/OnAdd handlers stashed
    // (Setup not yet run on the client, or the stash already non-empty) into ReplayQueue in arrival
    // order. Runs BEFORE ApplyReplicatedHistory so a released entry can drain in the same tick.

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
        using RunAfter      = TDepList<FProcessor_Sm_HandleRequests>;
        using RunBefore     = TDepList<FProcessor_Sm_ApplyReplicatedHistory>;
        using MarkedDirtyBy = FFragment_Sm_PendingReplicationEntries;

    public:
        using TProcessor::TProcessor;

    public:
        // FFragment_Sm_Current is TReadWrite because MirrorRunStatus mutates it when applying the
        // stashed run-status after events drain.
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_PendingReplicationEntries& InStash) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // APPLY REPLICATED HISTORY — non-owning client replay path: drains FFragment_Sm_ReplayQueue one
    // entry per tick into FFragment_Sm_PendingTransition for CommitPendingTransition to land. The
    // TExcludes keep it off an in-flight transition and off a determinism-faulted SM.

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

    // --------------------------------------------------------------------------------------------------------------------
    // COMMIT PENDING TRANSITION — defers the new state's entry until the previous state's exit
    // cascade has fully drained.

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
        using LocalSettleAfter = FGroup_Gameplay_Script;
        static constexpr auto LocalSettleTrigger = true;

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

    // --------------------------------------------------------------------------------------------------------------------
    // HYDRATION RESUME — authority-side save-load: walks a phase ladder (Start or Stop -> Transition
    // into the saved state -> re-Pause -> remove the fragment, which is the done marker) to converge
    // a freshly-composed StateMachine onto its SAVED run-state. Only machines where the LOCAL machine
    // is the request authority re-drive; the rest come back composed-but-Stopped.
    // --------------------------------------------------------------------------------------------------------------------

    class CKSTATEMACHINE_API FProcessor_Sm_HydrationResume : public ck_exp::TProcessor<
        FProcessor_Sm_HydrationResume,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_Current>,
        TReadWrite<FFragment_Sm_HydrationResume>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_Sm_HydrationResume;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sm_Params& InParams,
            FFragment_Sm_Current& InCurrent,
            FFragment_Sm_HydrationResume& InResume) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
    // PUSH OWNING-CLIENT BATCH — end-of-frame flush of the owning client's locally-buffered
    // transitions / run-status into a single RPC on the per-actor StateMachineRelay, after which
    // the standard server-side publication path takes over. ClientOnly NetModeRequirement: the
    // processor is created only on machines with FTag_NetMode_IsClient — servers never push to
    // themselves.

    class CKSTATEMACHINE_API FProcessor_Sm_PushOwningClientBatch : public ck_exp::TProcessor<
        FProcessor_Sm_PushOwningClientBatch,
        FCk_Handle_StateMachine,
        TReadOnly<FFragment_Sm_Params>,
        TReadWrite<FFragment_Sm_PendingClientBatch>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;

        // Intentionally NO MarkedDirtyBy: the relay channel is server-spawned and can take several
        // frames to replicate, so a push may DEFER without consuming the batch. A dirty gate would
        // only re-fire on a NEW transition, stranding the deferred batch forever. The fragment is
        // transient (removed on a successful push), so this iterates nothing on most frames.

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

    // --------------------------------------------------------------------------------------------------------------------

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
