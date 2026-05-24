#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmState_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmTask_EntityScript;
class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmState_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmState_EntityScript);

    // ================================================================================================================
    // LIFECYCLE (EntityScript overrides)
    // ================================================================================================================

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    // Child entities derive replication from their owning SM's params, not from the EntityScript
    // CDO default. When _AssociatedEntity is set and has an OwningStateMachine fragment, this
    // override defers to that SM's params._Replication so local-only SMs produce DoesNotReplicate
    // children even though the inherited CkEntityScript default is Replicates. CDO calls (before
    // the script is attached to an entity) fall back to Super.
    auto
    Get_EffectiveReplication() const -> ECk_Replication override;

    // ================================================================================================================
    // STATE LIFECYCLE (Enter/Exit)
    // ================================================================================================================
    //
    // EnterState fires from BeginPlay() once the state's script is fully constructed.
    // ExitState is invoked synchronously by the StateMachine processor — either by
    // FProcessor_Sm_HandleRequests on a transition, or by FProcessor_Sm_EndPlay when the SM
    // itself is destroyed mid-state. ExitState fires at most once per state instance, never
    // from the EntityScript EndPlay pipeline.

public:
    virtual auto
    EnterState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    virtual auto
    ExitState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    // ================================================================================================================
    // VIRTUAL METHODS (user overrides)
    // ================================================================================================================

protected:
    virtual auto
    DefineState(
        FCk_Handle_SmState_UnderConstruction& InHandle) -> void;

    // Returns the set of gameplay tags identifying the states this class overrides.
    // Called on the CDO when processing Request_AddOverrideState.
    // Default returns empty — override in subclasses that act as state overrides.
public:
    auto
    Get_StatesToOverride() const -> TArray<FGameplayTag>;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Define State")
    void
    DoDefineState(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Get States To Override")
    TArray<FGameplayTag>
    DoGet_StatesToOverride() const;

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Enter State")
    void
    DoEnterState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Exit State")
    void
    DoExitState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext);

    // ================================================================================================================
    // BUILDER API (call from DefineState only — enforced by UnderConstruction handle)
    // ================================================================================================================

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Task")
    FCk_Handle_SmTask
    AddTask(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Transition")
    FCk_Handle_SmTransition
    AddTransition(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) const;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Condition To Transition")
    FCk_Handle_SmCondition
    AddCondition(
        UPARAM(ref) FCk_Handle_SmTransition& InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass) const;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Compose From State")
    void
    ComposeFromState(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InOtherStateClass) const;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Remove Task")
    bool
    RemoveTask(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const;

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Replace Task")
    FCk_Handle_SmTask
    ReplaceTask(
        UPARAM(ref) FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InOldTaskClass,
        TSubclassOf<UCk_SmTask_EntityScript> InNewTaskClass) const;

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get State Tag For Class")
    FGameplayTag
    Get_StateTag() const;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get State Tag For Class")
    static FGameplayTag
    Get_StateTagForClass(
        TSubclassOf<UCk_SmState_EntityScript> InClass);

    // Per-class fingerprint cache (spec §9 verify path support).
    //
    // The structural fingerprint of a state class is computed from its DefineState output —
    // an instance operation that has to wait for the EntityScript Construct pipeline to run.
    // For replication publication we need the fingerprint *before* the new state's instance
    // exists, otherwise published transition events ship with fingerprint=0 and clients can't
    // verify determinism on commit.
    //
    // Resolution: cache the fingerprint per class on first compute. Every Construct's call to
    // DoComputeFingerprint populates this cache. CommitPendingTransition's publication path
    // reads it. The cache is class-name-keyed so hot reload + PIE-restart cycles don't dangle.
    // First-ever instantiation of a class still publishes 0 (cache miss); the Construct
    // backfill (DoBackfillFingerprintToRepData) cleans up that one zero asynchronously.
    //
    // Returns 0 when the class has never been seen — caller treats 0 as "no fingerprint, skip
    // verify" downstream.
    static auto
    Get_CachedFingerprint(
        TSubclassOf<UCk_SmState_EntityScript> InClass) -> int32;

    static auto
    Set_CachedFingerprint(
        TSubclassOf<UCk_SmState_EntityScript> InClass,
        int32 InFingerprint) -> void;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get Owner StateMachine",
        meta = (CompactNodeTitle = "OwnerSM", HideSelfPin = true))
    FCk_Handle_StateMachine
    Get_OwnerStateMachine() const;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get StateMachine Context",
        meta = (CompactNodeTitle = "Context", HideSelfPin = true))
    FCk_Handle
    Get_StateMachineContext() const;

    // ================================================================================================================
    // INTERNALS
    // ================================================================================================================

private:
    // Walks the state's records (tasks, transitions, conditions) plus the transient
    // ComposedFromInProgress fragment, computes the structural fingerprint, stores it on
    // FFragment_SmState_Fingerprint, and clears the transient scratch. Called by Construct
    // immediately after DefineState returns.
    auto
    DoComputeFingerprint(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // On authority + Replicates SMs, patches the locally-computed fingerprint into the SM's
    // replicated payload. Covers two cases: (1) initial-state seeds _InitialStateFingerprint
    // when history is empty; (2) subsequent transitions patch History.Last()._NewStateFingerprint
    // when that event's NewStateClass matches our class. WithoutHistory uses _CurrentStateFingerprint
    // gated on _CurrentStateClass match. No-op for non-authority, local-only, or non-matching
    // payload state (defensive against fast follow-up transitions racing past this Construct).
    auto
    DoBackfillFingerprintToRepData(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // Spec §9 determinism verify. If FProcessor_Sm_CommitPendingTransition attached
    // FFragment_SmState_ExpectedFingerprint to this state (meaning the commit was driven by a
    // replicated event with a non-zero fingerprint), compare to the locally-computed hash and
    // fault the SM on mismatch — exit the bad state and stamp FTag_Sm_DeterminismFault so no
    // further transitions land. Authority-driven commits never carry the carrier, so this is
    // a no-op for the authoritative machine.
    auto
    DoVerifyFingerprintAgainstExpected(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

private:
    FCk_Handle_StateMachine _OwnerStateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
