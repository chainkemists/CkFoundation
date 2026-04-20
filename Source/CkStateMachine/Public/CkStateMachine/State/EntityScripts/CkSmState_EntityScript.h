#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

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

    UCk_SmState_EntityScript()
    {
        _Replication = ECk_Replication::DoesNotReplicate;
    }

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
        FCk_Handle_SmState InHandle) -> void;

    virtual auto
    ExitState(
        FCk_Handle_SmState InHandle) -> void;

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
        FCk_Handle_SmState InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Exit State")
    void
    DoExitState(
        FCk_Handle_SmState InHandle);

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
    // MEMBERS
    // ================================================================================================================

private:
    FCk_Handle_StateMachine _OwnerStateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
