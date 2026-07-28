#pragma once

#include "CkStateMachine_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include <StructUtils/InstancedStruct.h>

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
    // --------------------------------------------------------------------------------------------------------------------

    // For a local-only SM, construct the params from just the initial state class — the defaults are
    // AutoStart=OnSetup / DoesNotReplicate / AuthorityModel=AutoDetect / WithHistory.
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

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Start",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_Start(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Stop",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_Stop(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Pause",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_Pause(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Resume",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_Resume(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Transition",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_Transition(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Schedules exit on the SM's current state via UCk_Utils_SmState_UE::Request_Exit. Does NOT
    // destroy the SM entity itself.
    static auto
    Request_ExitStateMachine(
        FCk_Handle_StateMachine& InStateMachine) -> FCk_Handle_StateMachine;

    // Discards an uncommitted pending transition: destroys the deliberately-kept-alive previous state
    // entity the fragment stashes, then removes the fragment. Every path that abandons a mid-flight
    // transition must go through this — removing the fragment alone leaks the entity. Internal.
    static auto
    Request_TryDiscardPendingTransition(
        FCk_Handle& InEntity) -> void;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Request Add Override State",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_StateMachine
    Request_AddOverrideState(
        UPARAM(ref) FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InOverrideStateClass,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // True while replicated/relayed transitions are still queued on this entity, or one is mid-commit.
    // Receive paths use it to defer non-Running run-status mirrors until the replay pipeline drains.
    static auto
    Get_HasReplayTransitionsInFlight(
        const FCk_Handle& InEntity) -> bool;

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

    // Resolved fresh from authority and ownership queries each call — the same value the SM's
    // EnterState/ExitState/EnterTask/ExitTask/Tick callbacks receive.
    UFUNCTION(BlueprintPure,
        Category = "Ck|StateMachine",
        DisplayName = "[Ck][SM] Get Net Context")
    static ECk_Sm_NetContext
    Get_NetContext(
        const FCk_Handle_StateMachine& InStateMachine);

    // Immutable per-SM choice from FFragment_Sm_Params._Replication. C++-only.
    static auto
    Get_Replication(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Replication;

    // Raw, authored per-SM choice — may be AutoDetect, so this is for introspection only. Authority
    // decisions use Get_EffectiveAuthorityModel.
    static auto
    Get_AuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_AuthorityModel;

    // Resolved authority model — NEVER returns AutoDetect. An explicit authored value is returned
    // as-is; AutoDetect resolves from the host's net ownership (player-controlled pawn ->
    // OwningClientAuthoritative, everything else -> ServerAuthoritative). ALL authority gates use this.
    static auto
    Get_EffectiveAuthorityModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_AuthorityModel;

    // Immutable per-SM choice from FFragment_Sm_Params._ReplicationModel — read to switch on payload shape.
    static auto
    Get_ReplicationModel(
        const FCk_Handle_StateMachine& InStateMachine) -> ECk_Sm_ReplicationModel;

    // --------------------------------------------------------------------------------------------------------------------
    // SUB-SM TRANSITION RELAY. Sub-SMs have no transport of their own, so an owning-client sub-SM's
    // transitions are routed through the ROOT SM's transport and addressed by a deterministic
    // identity: FFragment_Sm_ParentHierarchy, the root->leaf state-tag path to its hosting state.

    // Walks OwningStateMachine to the top-level SM; returns InStateMachine itself when already a root.
    static auto
    Get_RootStateMachine(
        const FCk_Handle_StateMachine& InStateMachine) -> FCk_Handle_StateMachine;

    // The cross-machine identity of a sub-SM. Empty for a root SM.
    static auto
    Get_SubSmParentHierarchy(
        const FCk_Handle_StateMachine& InStateMachine) -> TArray<FGameplayTag>;

    // Returns an invalid handle when no ACTIVE sub-SM matches — e.g. the hosting parent state isn't
    // currently active, in which case the caller must defer/stash.
    static auto
    TryFind_ActiveSubSm_ByParentHierarchy(
        const FCk_Handle_StateMachine& InRoot,
        const TArray<FGameplayTag>& InParentHierarchy) -> FCk_Handle_StateMachine;

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

    // Resolves the StateMachineRelay channel owned by the SM's owning player, via
    // UCk_StateMachineRelay_Subsystem_UE on the SM's World. Returns an invalid result when the
    // subsystem is missing or the owning channel isn't available yet (channels auto-spawn at
    // PostLogin, so callers early in the world's lifetime transiently see none).
    static auto
    Acquire_RelayChannel(
        const FCk_Handle_StateMachine& InStateMachine) -> FCk_ActorRelay_ChannelResult;

    // --------------------------------------------------------------------------------------------------------------------

    static void
    TryCheckEntryBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    static void
    TryCheckExitBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

private:
    static auto
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate) -> FCk_Handle_StateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
