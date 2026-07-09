#pragma once

#include "CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <StructUtils/InstancedStruct.h>

// Per-feature Utils headers — included here for backward compatibility
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"

#include "CkStateMachine_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_StateMachine"))
class CKSTATEMACHINE_API UCk_Utils_StateMachine_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_StateMachine_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_StateMachine);

public:
    struct RecordOfSmStates_Utils      : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSmStates> {};
    struct RecordOfSmTransitions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSmTransitions> {};
    struct RecordOfSmTasks_Utils       : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSmTasks> {};
    struct RecordOfSmConditions_Utils  : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSmConditions> {};

public:
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    // Adds a state machine to InOwner from a params struct. For a local-only SM, construct the
    // params from just the initial state class — FCk_Fragment_StateMachine_ParamsData(InitialState) —
    // which defaults to AutoStart=OnSetup / DoesNotReplicate / AuthorityModel=AutoDetect / WithHistory.
    // Opt into replication / authority / replication-model via the struct's setters.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Add StateMachine")
    static FCk_Handle_StateMachine
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_StateMachine_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Create")
    static FCk_Handle_StateMachine
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_StateMachine_ParamsData& InParams);

    // ================================================================================================================
    // CONTROL
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Start")
    static FCk_Handle_StateMachine
    Request_Start(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Stop")
    static FCk_Handle_StateMachine
    Request_Stop(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Pause")
    static FCk_Handle_StateMachine
    Request_Pause(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Resume")
    static FCk_Handle_StateMachine
    Request_Resume(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Transition")
    static FCk_Handle_StateMachine
    Request_Transition(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass);

    // Schedules exit on the SM's current state via UCk_Utils_SmState_UE::Request_Exit (adds
    // FTag_SmState_PendingExit + destroys the state entity). Does NOT destroy the SM entity.
    // Used by UCk_SmTask_SubStateMachine::ExitTask to propagate exit into an active sub-SM.
    static auto
    Request_ExitStateMachine(
        FCk_Handle_StateMachine& InStateMachine) -> FCk_Handle_StateMachine;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Add Override State")
    static FCk_Handle_StateMachine
    Request_AddOverrideState(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InOverrideStateClass);

    // ================================================================================================================
    // GETTERS
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Run Status")
    static ECk_SmRunStatus
    Get_RunStatus(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Current State Class")
    static TSubclassOf<UCk_SmState_EntityScript>
    Get_CurrentStateClass(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Current State Handle")
    static FCk_Handle_SmState
    Get_CurrentStateHandle(
        const FCk_Handle_StateMachine& InStateMachine);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Is In State")
    static bool
    IsInState(
        const FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    // Returns the SM's current NetContext (Standalone / Server / OwningClient / NonOwningClient),
    // resolved fresh from authority and ownership queries each call. Same value the SM's
    // EnterState/ExitState/EnterTask/ExitTask/Tick callbacks receive.
    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Net Context")
    static ECk_Sm_NetContext
    Get_NetContext(
        const FCk_Handle_StateMachine& InStateMachine);

    // Immutable per-SM choice from FFragment_Sm_Params._Replication. Local-only SMs return
    // DoesNotReplicate; replicated SMs return Replicates. C++-only at this stage to avoid the
    // AS-binding refresh quirk that bites newly-added BPFL UFUNCTIONs in the same toolbox run.
    static auto
    Get_Replication(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Replication;

    // Raw, authored per-SM choice from FFragment_Sm_Params._AuthorityModel — may be AutoDetect.
    // For authority decisions use Get_EffectiveAuthorityModel; this getter is for introspection
    // (debugger / tooling that wants to show what was authored).
    static auto
    Get_AuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_AuthorityModel;

    // Resolved authority model — NEVER returns AutoDetect. If the authored value is explicit
    // (ServerAuthoritative / OwningClientAuthoritative) it is returned as-is. AutoDetect resolves
    // from the SM host's net ownership: a player-controlled pawn host -> OwningClientAuthoritative,
    // everything else (bots, non-pawn / non-actor-bridged entities) -> ServerAuthoritative.
    // Resolution is lazy/on-demand, so the host's PlayerState/ownership is settled by the time the
    // authority gates read it. ALL authority gates must use this, not Get_AuthorityModel.
    static auto
    Get_EffectiveAuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_AuthorityModel;

    // Immutable per-SM choice from FFragment_Sm_Params._ReplicationModel. WithHistory by default;
    // WithoutHistory for snap-to-current SMs. Read by Phase 6+ to switch on payload shape.
    static auto
    Get_ReplicationModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_ReplicationModel;

    // ================================================================================================================
    // SUB-SM TRANSITION RELAY (identity + resolution)
    // ================================================================================================================
    //
    // Sub-SMs are non-replicated local entities (created by UCk_SmTask_SubStateMachine), so they have
    // no transport of their own. To replicate an owning-client sub-SM's transitions to the server /
    // non-owning clients, the events are routed through the ROOT SM's transport (the root rides the
    // pawn entity's replication driver) and addressed by a deterministic identity: the sub-SM's
    // FFragment_Sm_ParentHierarchy (root->leaf state-tag path to its hosting state). The topology is
    // deterministic across machines, so the same path resolves the corresponding sub-SM on every peer.

    // Walk OwningStateMachine to the top-level SM. Returns InStateMachine itself when it is already a
    // root (no owning SM). C++-only — internal to the replication routing.
    static auto
    Get_RootStateMachine(
        const FCk_Handle_StateMachine& InStateMachine) -> FCk_Handle_StateMachine;

    // The cross-machine identity of a sub-SM: its parent hierarchy path. Empty for a root SM (a root
    // has no FFragment_Sm_ParentHierarchy). C++-only.
    static auto
    Get_SubSmParentHierarchy(
        const FCk_Handle_StateMachine& InStateMachine) -> TArray<FGameplayTag>;

    // Resolve the local sub-SM under InRoot whose parent hierarchy equals InParentHierarchy, by
    // walking the active state tree (current state -> hosted sub-SMs, recursively). Returns an invalid
    // handle if no active sub-SM matches (e.g. the hosting parent state isn't currently active — the
    // caller must defer/stash in that case). C++-only.
    static auto
    TryFind_ActiveSubSm_ByParentHierarchy(
        const FCk_Handle_StateMachine& InRoot,
        const TArray<FGameplayTag>& InParentHierarchy) -> FCk_Handle_StateMachine;

    // ================================================================================================================
    // SIGNAL BINDING
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStateChanged")
    static FCk_Handle_StateMachine
    BindTo_OnStateChanged(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStateChanged")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStateChanged(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStarted")
    static FCk_Handle_StateMachine
    BindTo_OnStarted(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStarted")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStarted(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStarted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnStopped")
    static FCk_Handle_StateMachine
    BindTo_OnStopped(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnStopped")
    static FCk_Handle_StateMachine
    UnbindFrom_OnStopped(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Sm_OnStopped& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Bind To OnTaskFinished")
    static FCk_Handle_SmTask
    BindTo_OnSmTaskFinished(
        UPARAM(ref) FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Unbind From OnTaskFinished")
    static FCk_Handle_SmTask
    UnbindFrom_OnSmTaskFinished(
        UPARAM(ref) FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnFinished& InDelegate);

    // ================================================================================================================
    // REPLICATION RELAY (Phase 4 — stub. Phase 10 wires consumption from OwningClient path.)
    // ================================================================================================================

    // Resolves the StateMachineRelay channel for an SM that opts into OwningClientAuthoritative.
    // Looks up UCk_StateMachineRelay_Subsystem_UE on the SM's World and acquires a channel via
    // the actor-relay group infrastructure. Returns an invalid result if the subsystem is missing
    // or no channel is available yet (the subsystem auto-spawns channels at PostLogin so callers
    // very early in the world's lifetime may transiently see no channel).
    //
    // Phase 10 will refine this to prefer the channel owned by the SM's owning PlayerState; for
    // now it returns whichever channel the subsystem assigns via its selection algorithm, which
    // is sufficient for Phase 4's compile-only goal.
    static auto
    Acquire_RelayChannel(
        const FCk_Handle_StateMachine& InStateMachine) -> FCk_ActorRelay_ChannelResult;

    // ================================================================================================================
    // DEBUG
    // ================================================================================================================

    static void
    TryCheckEntryBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    static void
    TryCheckExitBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    // ================================================================================================================
    // CAST
    // ================================================================================================================

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_StateMachine
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Handle -> SM Handle",
        meta = (CompactNodeTitle = "<AsSM>", BlueprintAutocast))
    static FCk_Handle_StateMachine
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid SM Handle",
        Category = "Ck|StateMachine",
        meta = (CompactNodeTitle = "INVALID_SmHandle", Keywords = "make"))
    static FCk_Handle_StateMachine
    Get_InvalidHandle() { return {}; }

    // ================================================================================================================
    // INTERNALS
    // ================================================================================================================

private:
    static auto
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest) -> FCk_Handle_StateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
