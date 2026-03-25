#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

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

    // ================================================================================================================
    // VIRTUAL METHODS (user overrides)
    // ================================================================================================================

protected:
    virtual auto
    DefineState(
        FCk_Handle& InHandle) -> void;

    virtual auto
    OnStateEnter() -> void;

    virtual auto
    OnStateExit() -> void;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "Define State")
    void
    DoDefineState(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "On State Enter")
    void
    DoOnStateEnter(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|State",
        DisplayName = "On State Exit")
    void
    DoOnStateExit(
        FCk_Handle InHandle);

    // ================================================================================================================
    // BUILDER API (call from DefineState)
    // ================================================================================================================

public:
    auto
    AddTask(
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) -> FCk_Handle_SmTask;

    auto
    AddTransition(
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> FCk_Handle_SmTransition;

    static auto
    AddCondition(
        FCk_Handle_SmTransition InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass) -> FCk_Handle_SmCondition;

    // ================================================================================================================
    // UFUNCTION BUILDER API (Blueprint/AngelScript)
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Task")
    FCk_Handle_SmTask
    DoAddTask(
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Transition")
    FCk_Handle_SmTransition
    DoAddTransition(
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Add Condition To Transition")
    FCk_Handle_SmCondition
    DoAddCondition(
        FCk_Handle_SmTransition InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass);

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get Owner StateMachine",
        meta = (CompactNodeTitle = "OwnerSM", HideSelfPin = true))
    FCk_Handle_StateMachine
    DoGet_OwnerStateMachine() const;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|State",
        DisplayName = "[Ck][SM] Get Game Entity",
        meta = (CompactNodeTitle = "GameEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_GameEntity() const;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

private:
    FCk_Handle_StateMachine _OwnerStateMachine;
    int32 _TransitionOrderCounter = 0;

public:
    CK_PROPERTY_GET(_OwnerStateMachine);
};

// --------------------------------------------------------------------------------------------------------------------
