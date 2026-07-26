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

    // --------------------------------------------------------------------------------------------------------------------

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    // Always DoesNotReplicate: state/condition/task entities are never independent net objects — the
    // SM's transition-history container is the only thing on the wire and clients rebuild the whole
    // sub-graph locally via the replay path.
    auto
    Get_EffectiveReplication() const -> ECk_Replication override;

    // --------------------------------------------------------------------------------------------------------------------

    // EnterState fires from BeginPlay() once the script is fully constructed. ExitState is invoked
    // synchronously by the SM processors (transition, or SM EndPlay mid-state), with an
    // EntityScript-EndPlay fallback for cascade-destroyed states the processors never reach.
    // ExitState fires at most once per state instance (dedup'd via FTag_SmState_Active).

public:
    virtual auto
    EnterState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    virtual auto
    ExitState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    // --------------------------------------------------------------------------------------------------------------------

protected:
    virtual auto
    DefineState(
        FCk_Handle_SmState_UnderConstruction& InHandle) -> void;

    // Gameplay tags identifying the states this class overrides; called on the CDO when processing
    // Request_AddOverrideState. Empty by default — override in subclasses that act as state overrides.
public:
    auto
    Get_StatesToOverride() const -> TArray<FGameplayTag>;

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

    // Builder API — callable from DefineState only; enforced by the UnderConstruction handle type.

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

    // Per-class fingerprint cache: publication needs a class's fingerprint BEFORE the new state's
    // instance exists, and every Construct's DoComputeFingerprint populates this. Returns 0 for a
    // class never seen — callers treat 0 as "no fingerprint, skip verify".
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

    // --------------------------------------------------------------------------------------------------------------------

private:
    // Walks the state's records plus the transient ComposedFromInProgress fragment, stores the
    // structural fingerprint on FFragment_SmState_Fingerprint, and clears the scratch.
    auto
    DoComputeFingerprint(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // On authority + Replicates SMs, patches the locally-computed fingerprint into the SM's rep
    // payload: seeds _InitialStateFingerprint when history is empty, otherwise patches
    // History.Last() (WithHistory) / _CurrentStateFingerprint (WithoutHistory) on a class match.
    auto
    DoBackfillFingerprintToRepData(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // Determinism verify: when FFragment_SmState_ExpectedFingerprint was attached (the commit was
    // driven by a replicated event), compare it to the local hash and on mismatch exit the state and
    // stamp FTag_Sm_DeterminismFault. Authority commits never carry the carrier, so it no-ops there.
    auto
    DoVerifyFingerprintAgainstExpected(
        FCk_Handle_SmState_UnderConstruction& InStateHandle) -> void;

    // --------------------------------------------------------------------------------------------------------------------

private:
    FCk_Handle_StateMachine _OwnerStateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
