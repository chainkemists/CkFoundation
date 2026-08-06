#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include <StructUtils/InstancedStruct.h>

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/Task/CkSmTask_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmTask_EntityScript;
class UCk_SmTask_SubStateMachine;
class UCk_EntityScript_UE;
class UCk_Utils_StateMachine_UE;
class UCk_Utils_SmTask_UE;
class UCk_Utils_SmCondition_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_SmState_Evaluate;

    class FProcessor_Sm_PushOwningClientBatch;
    class FProcessor_Sm_FlushPendingReplication_Drain;
    class FProcessor_Sm_ApplyReplicatedHistory;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Sm_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Sm_Running);
    CK_DEFINE_ECS_TAG(FTag_Sm_Paused);

    // Marks a sub-state-machine (created by UCk_SmTask_SubStateMachine, owned by a parent SM's task).
    // Top-level SMs never carry it.
    CK_DEFINE_ECS_TAG(FTag_Sm_IsSubMachine);

    // Marks an SM child entity (Task/Condition) whose EntityScript attach is deferred until
    // FProcessor_SmScript_CommitPendingAttach materializes it. Strip this tag +
    // FFragment_SmScript_PendingAttach to cancel the attach before commit runs.
    CK_DEFINE_ECS_TAG(FTag_SmScript_PendingAttach);

    // --------------------------------------------------------------------------------------------------------------------

    // Sticky fault tag set by UCk_SmState_EntityScript::DoVerifyFingerprintAgainstExpected when a
    // replayed transition's fingerprint diverges from the locally-computed one. ApplyReplicatedHistory
    // TExcludes it, so the SM is permanently quiesced. Only the transition-time verify sets it.
    CK_DEFINE_ECS_TAG(FTag_Sm_DeterminismFault);

    // One-shot trigger added by MirrorRunStatus when a NON-AUTHORITY machine first learns the SM is
    // Running but has no current state. Consumed by FProcessor_Sm_FirstSyncInitialState, which enters
    // the locally-known initial state.
    CK_DEFINE_ECS_TAG(FTag_Sm_NeedsInitialStateEntry);

    // --------------------------------------------------------------------------------------------------------------------

    // Carries the EntityScript class to attach to an SM child entity (Task/Condition) when the
    // commit processor runs.
    struct CKSTATEMACHINE_API FFragment_SmScript_PendingAttach
    {
    public:
        CK_GENERATED_BODY(FFragment_SmScript_PendingAttach);

        friend class FProcessor_SmScript_CommitPendingAttach;
        friend class ::UCk_Utils_SmTask_UE;
        friend class ::UCk_Utils_SmCondition_UE;

    private:
        TSubclassOf<UCk_EntityScript_UE> _ScriptClass;

    public:
        CK_PROPERTY_GET(_ScriptClass);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmScript_PendingAttach, _ScriptClass);
    };

    // --------------------------------------------------------------------------------------------------------------------


    using FFragment_Sm_Params = FCk_StateMachine_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Current);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_CommitPendingTransition;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_Sm_EndPlay;
        friend class FProcessor_Sm_HydrationResume;
        friend class ::UCk_Utils_StateMachine_UE;

    private:
        ECk_SmRunStatus _RunStatus = ECk_SmRunStatus::Stopped;
        FCk_Handle_SmState _CurrentStateHandle;
        TSubclassOf<UCk_SmState_EntityScript> _CurrentStateClass;

    public:
        // _RunStatus is CK_PROPERTY (not GET) because the client-side run-status mirror in
        // CkStateMachine_Replication.cpp needs to write it from outside the friend list.
        CK_PROPERTY(_RunStatus);
        CK_PROPERTY_GET(_CurrentStateHandle);
        CK_PROPERTY_GET(_CurrentStateClass);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_Current, _RunStatus, _CurrentStateHandle, _CurrentStateClass);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Presence on an SM means "transition mid-flight": the previous state's Request_Exit has
    // been issued but the new state has not been entered. FProcessor_Sm_CommitPendingTransition
    // filters on this fragment, lands the entry once the exit cascade has drained, then removes it.
    struct CKSTATEMACHINE_API FFragment_Sm_PendingTransition
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_PendingTransition);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_CommitPendingTransition;
        friend class FProcessor_Sm_ApplyReplicatedHistory;

    private:
        FCk_Handle_SmState _PreviousStateHandle;
        TSubclassOf<UCk_SmState_EntityScript> _PreviousStateClass;
        TSubclassOf<UCk_SmState_EntityScript> _TargetStateClass;

        // Fingerprint carried from the replicated event (set by ApplyReplicatedHistory); the commit
        // processor verifies the local DefineState hash against it post-Construct. 0 is the sentinel
        // for "server-originated, no verification".
        int32 _NewStateFingerprint = 0;

    public:
        CK_PROPERTY(_PreviousStateHandle);
        CK_PROPERTY(_PreviousStateClass);
        CK_PROPERTY(_TargetStateClass);
        CK_PROPERTY(_NewStateFingerprint);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Hierarchy prefix applied to every state spawned within this StateMachine.
    // Empty for top-level SMs. Seeded by sub-StateMachine spawners with the hosting state's hierarchy.
    struct CKSTATEMACHINE_API FFragment_Sm_ParentHierarchy
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_ParentHierarchy);

    private:
        TArray<FGameplayTag> _ParentHierarchy;

    public:
        CK_PROPERTY_GET(_ParentHierarchy);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_ParentHierarchy, _ParentHierarchy);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // C++-only. Holds the list of state-override entries installed on this StateMachine.
    // Consulted inside UCk_Utils_SmState_UE::Create to swap the spawned class.
    // Each entry caches the tags from the override class's Get_StatesToOverride() CDO call.
    struct CKSTATEMACHINE_API FFragment_Sm_StateOverrides
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_StateOverrides);

        friend class FProcessor_Sm_HandleRequests;
        friend class ::UCk_Utils_StateMachine_UE;
        friend class ::UCk_Utils_SmState_UE;

        struct FEntry
        {
            TSubclassOf<UCk_SmState_EntityScript> _OverrideStateClass;
            TArray<FGameplayTag> _CachedStatesToOverride;
        };

    private:
        TArray<FEntry> _Overrides;

    public:
        CK_PROPERTY_GET(_Overrides);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Requests);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_SmState_Evaluate;
        friend class ::UCk_Utils_StateMachine_UE;

        using RequestType = std::variant<
            FCk_Request_Sm_Start,
            FCk_Request_Sm_Stop,
            FCk_Request_Sm_Pause,
            FCk_Request_Sm_Resume,
            FCk_Request_Sm_Transition,
            FCk_Request_Sm_AddOverrideState
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Transient phase ladder for the save-load SM hydration redrive (FProcessor_Sm_HydrationResume).
    // Populated by the SM's save-transport Apply handler with the saved {RunStatus, CurrentStateClass};
    // removes itself (the done marker) once the SM converges. Never persisted.
    struct CKSTATEMACHINE_API FFragment_Sm_HydrationResume
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_HydrationResume);

        friend class FProcessor_Sm_HydrationResume;

        enum class EPhase : uint8
        {
            Start,        // drive RunStatus toward the desired value (Start, or Stop if AutoStart resurrected a saved-Stopped SM)
            Transition,   // once Running: transition InitialState -> saved state (replay-through)
            Finalize      // re-apply Paused if that's what was saved, then mark done
        };

    private:
        ECk_SmRunStatus _DesiredRunStatus = ECk_SmRunStatus::Stopped;
        TSubclassOf<UCk_SmState_EntityScript> _DesiredStateClass;

        EPhase _Phase = EPhase::Start;

        // One-shot request guards — each Request_* is enqueued exactly once; the ladder then
        // observes FFragment_Sm_Current until the request's effect lands (requests are deferred).
        bool _StartEnqueued = false;
        bool _StopEnqueued = false;
        bool _TransitionEnqueued = false;
        bool _PauseEnqueued = false;

    public:
        // Seeded by the save-transport Apply handler, which is not a friend.
        auto
        Populate(
            ECk_SmRunStatus                       InDesiredRunStatus,
            TSubclassOf<UCk_SmState_EntityScript> InDesiredStateClass) -> void
        {
            _DesiredRunStatus  = InDesiredRunStatus;
            _DesiredStateClass = InDesiredStateClass;
        }

    public:
        CK_PROPERTY_GET(_DesiredRunStatus);
        CK_PROPERTY_GET(_DesiredStateClass);
        CK_PROPERTY_GET(_Phase);
        CK_PROPERTY_GET(_StartEnqueued);
        CK_PROPERTY_GET(_StopEnqueued);
        CK_PROPERTY_GET(_TransitionEnqueued);
        CK_PROPERTY_GET(_PauseEnqueued);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Resolved-once net identity, present only on SUB-SMs — a sub-SM is detached from the pawn, so
    // the live ComputeNetContext / Get_EffectiveAuthorityModel queries misresolve on it. When present
    // those queries return these stored values instead. The NetContext half is frozen at EnterTask.
    struct CKSTATEMACHINE_API FFragment_Sm_NetIdentity
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_NetIdentity);

    private:
        ECk_Sm_AuthorityModel _EffectiveAuthority = ECk_Sm_AuthorityModel::AutoDetect;
        ECk_Sm_NetContext     _NetContext         = ECk_Sm_NetContext::Standalone;

    public:
        CK_PROPERTY_GET(_EffectiveAuthority);
        CK_PROPERTY_GET(_NetContext);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_NetIdentity, _EffectiveAuthority, _NetContext);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-frame memo of the live net-context / transition-authority resolution, which walks
    // owning-actor/PlayerState chains and is queried several times per SM element per frame.
    // GFrameCounter-stamped, so no invalidation hook is needed.
    struct CKSTATEMACHINE_API FFragment_Sm_NetContextMemo
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_NetContextMemo);

    private:
        uint64            _NetContextFrame = MAX_uint64;
        ECk_Sm_NetContext _NetContext      = ECk_Sm_NetContext::Standalone;

        uint64 _TransitionAuthorityFrame = MAX_uint64;
        bool   _IsTransitionAuthority    = false;

    public:
        CK_PROPERTY(_NetContextFrame);
        CK_PROPERTY(_NetContext);
        CK_PROPERTY(_TransitionAuthorityFrame);
        CK_PROPERTY(_IsTransitionAuthority);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Owning-client outbound buffer, filled by FProcessor_Sm_CommitPendingTransition (transitions)
    // and DoPublishRunStatus (run-status). Flushed end-of-frame by FProcessor_Sm_PushOwningClientBatch
    // via Server_PushTransitionBatch / Server_PushRunStatus.
    struct CKSTATEMACHINE_API FFragment_Sm_PendingClientBatch
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_PendingClientBatch);

        friend class FProcessor_Sm_PushOwningClientBatch;
        friend class FProcessor_Sm_CommitPendingTransition;

    private:
        TArray<FCk_Sm_TransitionEvent> _PendingEvents;

        ECk_SmRunStatus _PendingRunStatus = ECk_SmRunStatus::Stopped;
        bool            _HasPendingRunStatus = false;

    public:
        CK_PROPERTY(_PendingEvents);
        CK_PROPERTY(_PendingRunStatus);
        CK_PROPERTY(_HasPendingRunStatus);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Non-replicated client-side state for tracking which seq we've applied locally.
    // Lives on every client (including owning client, though owning client suppresses OnChange).
    struct CKSTATEMACHINE_API FFragment_Sm_ClientReplayState
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_ClientReplayState);

        friend class FProcessor_Sm_ApplyReplicatedHistory;
        friend class FProcessor_Sm_FlushPendingReplication_Drain;

    private:
        int32 _ClientLastAppliedSeq = 0;

    public:
        CK_PROPERTY(_ClientLastAppliedSeq);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Parks a Paused/Stopped run-status mirror that arrived while replayed transitions are still
    // queued or mid-commit — applying it immediately would make FProcessor_Sm_CommitPendingTransition
    // DISCARD the queued transition. Applied (latest-wins) by the commit tail once the queue drains.
    struct CKSTATEMACHINE_API FFragment_Sm_DeferredRunStatusMirror
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_DeferredRunStatusMirror);

    private:
        ECk_SmRunStatus _Status = ECk_SmRunStatus::Stopped;

    public:
        CK_PROPERTY(_Status);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Monotonic seq counter. Lives on server for ServerAuth SMs, on owning-client for
    // OwningClientAuth SMs. Bumped each time a transition is published.
    struct CKSTATEMACHINE_API FFragment_Sm_NextSeq
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_NextSeq);

        friend class FProcessor_Sm_CommitPendingTransition;

    private:
        int32 _Next = 1;  // First seq is 1; 0 is reserved as "never applied"

    public:
        CK_PROPERTY(_Next);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Stash for rep payloads that arrive before local Setup completes OR while a prior stash
    // is still draining (stash-precedence invariant). Drained in arrival order by
    // FProcessor_Sm_FlushPendingReplication_Drain.
    struct CKSTATEMACHINE_API FFragment_Sm_PendingReplicationEntries
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_PendingReplicationEntries);

        friend class FProcessor_Sm_FlushPendingReplication_Drain;

    private:
        TArray<FCk_Sm_TransitionEvent> _StashedEntries;

        // Latest run-status from any rep payload that arrived while stashing, applied by Drain after
        // the stashed events queue. The bool distinguishes "never observed" from "observed with an
        // empty stash".
        ECk_SmRunStatus _PendingRunStatus = ECk_SmRunStatus::Stopped;
        bool            _HasPendingRunStatus = false;

    public:
        CK_PROPERTY(_StashedEntries);
        CK_PROPERTY(_PendingRunStatus);
        CK_PROPERTY(_HasPendingRunStatus);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // FIFO of replicated transition events drained one-per-pump by
    // FProcessor_Sm_ApplyReplicatedHistory.
    struct CKSTATEMACHINE_API FFragment_Sm_ReplayQueue
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_ReplayQueue);

        friend class FProcessor_Sm_ApplyReplicatedHistory;
        friend class FProcessor_Sm_FlushPendingReplication_Drain;

    private:
        TArray<FCk_Sm_TransitionEvent> _Queue;

    public:
        CK_PROPERTY(_Queue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(TUtils_Sm_OwningStateMachine, FFragment_Sm_OwningStateMachine, FCk_Handle_StateMachine);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStateChanged,
        FCk_Delegate_Sm_OnStateChanged,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStateChanged);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStarted,
        FCk_Delegate_Sm_OnStarted,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStarted);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmStopped,
        FCk_Delegate_Sm_OnStopped,
        FCk_Handle_StateMachine,
        FCk_Sm_Payload_OnStopped);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSTATEMACHINE_API,
        OnSmTaskFinished,
        FCk_Delegate_SmTask_OnFinished,
        FCk_Handle_SmTask,
        ECk_SmTaskResult);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Sm_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
