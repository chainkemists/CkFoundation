#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include <StructUtils/InstancedStruct.h>

// Per-feature fragment headers — included here for backward compatibility
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

    // Forward decls for replication-related processors (defined in later phases).
    class FProcessor_Sm_PushOwningClientBatch;
    class FProcessor_Sm_FlushPendingReplication_Drain;
    class FProcessor_Sm_FlushPendingReplication_InitialCheck;
    class FProcessor_Sm_ApplyReplicatedHistory;

    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_Sm_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Sm_Running);
    CK_DEFINE_ECS_TAG(FTag_Sm_Paused);

    // Marks an SM child entity (Task/Condition) whose user-authored EntityScript has been
    // deferred. A commit processor materializes the script before EntityScript processors
    // see the entity. Strip this tag + FFragment_SmScript_PendingAttach to cancel the
    // attach (e.g. when the child is removed before commit runs).
    CK_DEFINE_ECS_TAG(FTag_SmScript_PendingAttach);

    // ================================================================================================================
    // REPLICATION TAGS
    // ================================================================================================================

    // Sticky fault tag set by FProcessor_Sm_FlushPendingReplication_InitialCheck when the
    // initial-state fingerprint check (spec §9.5) detects local vs replicated divergence.
    // Excluded by both Flush_InitialCheck and ApplyReplicatedHistory via TExclude, so the
    // SM is permanently quiesced after the fault fires.
    CK_DEFINE_ECS_TAG(FTag_Sm_DeterminismFault);

    // One-shot trigger tag added by the RegisterLazy OnAdd handler when the RepData fragment
    // first arrives on the entity. Consumed (removed) by FProcessor_Sm_FlushPendingReplication_InitialCheck
    // before the fingerprint check evaluates.
    CK_DEFINE_ECS_TAG(FTag_Sm_NeedsInitialFingerprintCheck);

    // Sticky idempotence-guard tag added by FProcessor_Sm_FlushPendingReplication_InitialCheck
    // after the initial-state fingerprint check runs (whether it passed or failed). Prevents
    // FTag_Sm_NeedsInitialFingerprintCheck from re-arming on a subsequent OnAdd delivery
    // (e.g., net-relevance flip).
    CK_DEFINE_ECS_TAG(FTag_Sm_InitialFingerprintCheckCompleted);

    // One-shot trigger added by MirrorRunStatus when a NON-AUTHORITY machine first learns the SM
    // is Running but has no current state. The authority enters its initial state via DoStart;
    // non-authority machines never run Start (it's authority-only) and the initial entry isn't a
    // replayed transition, so without this they'd sit at <none> until the first transition.
    // Consumed (removed) by FProcessor_Sm_FirstSyncInitialState, which enters the locally-known
    // initial state so non-owning views reflect it immediately.
    CK_DEFINE_ECS_TAG(FTag_Sm_NeedsInitialStateEntry);

    // ================================================================================================================
    // FRAGMENTS
    // ================================================================================================================

    // Carries the EntityScript class (and optional spawn params) to attach to an SM child
    // entity (Task/Condition) when the commit processor runs. Deferring the attach avoids
    // a same-frame race with FProcessor_EntityScript_ContinueConstruction when the child
    // is removed before BeginPlay runs — see CkEntityLifetime_Fragment.cpp destruction
    // pipeline, where CK_IGNORE_PENDING_KILL does NOT exclude FTag_DestroyEntity_Initiate.
    struct CKSTATEMACHINE_API FFragment_SmScript_PendingAttach
    {
    public:
        CK_GENERATED_BODY(FFragment_SmScript_PendingAttach);

        friend class FProcessor_SmScript_CommitPendingAttach;
        friend class ::UCk_Utils_SmTask_UE;
        friend class ::UCk_Utils_SmCondition_UE;

    private:
        TSubclassOf<UCk_EntityScript_UE> _ScriptClass;
        FInstancedStruct                 _SpawnParams;

    public:
        CK_PROPERTY_GET(_ScriptClass);
        CK_PROPERTY_GET(_SpawnParams);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_SmScript_PendingAttach, _ScriptClass, _SpawnParams);
    };

    // --------------------------------------------------------------------------------------------------------------------


    using FFragment_Sm_Params = FCk_Fragment_StateMachine_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Current);

        friend class FProcessor_Sm_HandleRequests;
        friend class FProcessor_Sm_CommitPendingTransition;
        friend class FProcessor_Sm_Setup;
        friend class FProcessor_Sm_EndPlay;
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

        // Fingerprint carried from the replicated event (set by ApplyReplicatedHistory). The
        // commit processor uses it post-Construct to verify the non-authority machine's local
        // DefineState produced the same structural hash as authority (spec §9). Server-originated
        // transitions (HandleRequests path) leave this at 0 — that's the sentinel for "no
        // verification, this is the authoritative branch".
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

    // Resolved-once net identity, present only on SUB-SMs. A sub-SM entity is created detached
    // from the pawn (Request_CreateEntity under the task entity), so it never carries
    // FFragment_OwningActor_Current. The live ComputeNetContext / Get_EffectiveAuthorityModel
    // queries need the owning pawn (non-recursive owning-actor lookup) and therefore misresolve
    // on a sub-SM — it would see itself as NonOwningClient on the owning client and AutoDetect to
    // ServerAuthoritative. The SubStateMachine task snapshots the PARENT SM's live-resolved
    // identity into this fragment at EnterTask. EnterTask runs on every machine and the parent is
    // fully resolved by then, so each machine captures its own correct per-machine role. When
    // present, ComputeNetContext / Get_EffectiveAuthorityModel return these stored values instead
    // of resolving live. Top-level SMs never carry it (they hold the pawn and resolve live).
    // Nesting chains automatically: a nested sub-SM reads its parent sub-SM's already-stored value.
    //
    // Snapshot caveat: the per-machine NetContext is frozen at EnterTask. A mid-life re-possession
    // of the owning pawn would leave it stale; sub-SMs are expected not to outlive a possession
    // change. The EffectiveAuthority half is machine-independent and immutable, so it is never stale.
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

    // ================================================================================================================
    // REPLICATION FRAGMENTS
    // ================================================================================================================

    // Owning-client outbound buffer. Filled by FProcessor_Sm_CommitPendingTransition on the
    // owning client when it lands a transition on an OwningClientAuthoritative SM, and by
    // DoPublishRunStatus when the owning client changes run-status (Start/Stop/Pause/Resume).
    // Flushed end-of-frame via FProcessor_Sm_PushOwningClientBatch → Server_PushTransitionBatch /
    // Server_PushRunStatus RPCs.
    //
    // Run-status relay note: before this was added, owning-client run-status changes only wrote the
    // server→client rep container (a no-op on the client, which doesn't own it), so Server_PushRunStatus
    // was dead code and the server's OwningClientAuth SM never started. ApplyReplicatedHistory then
    // dropped every relayed transition (it requires _RunStatus == Running). Relaying run-status fixes
    // the owning-client authority path end-to-end.
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
        friend class FProcessor_Sm_FlushPendingReplication_InitialCheck;

    private:
        int32 _ClientLastAppliedSeq = 0;

    public:
        CK_PROPERTY(_ClientLastAppliedSeq);
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
    // is still draining (stash-precedence invariant, spec §5.4). Drained in arrival order by
    // FProcessor_Sm_FlushPendingReplication_Drain.
    struct CKSTATEMACHINE_API FFragment_Sm_PendingReplicationEntries
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_PendingReplicationEntries);

        friend class FProcessor_Sm_FlushPendingReplication_Drain;

    private:
        TArray<FCk_Sm_TransitionEvent> _StashedEntries;

        // Latest run-status from any rep payload that arrived while stashing. Applied by Drain
        // after the stashed events are queued. The bool gate distinguishes "no run-status was
        // ever observed during stash" (don't mirror) from "stash was empty but RunStatus update
        // was received during the same stash window" (mirror without touching events).
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

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_OwningStateMachine, FFragment_Sm_OwningStateMachine, FCk_Handle_StateMachine);

    // ================================================================================================================
    // SIGNALS
    // ================================================================================================================

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

    // ================================================================================================================

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Sm_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
