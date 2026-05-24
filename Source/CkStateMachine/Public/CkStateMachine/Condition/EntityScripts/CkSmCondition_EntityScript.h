#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_EntityScript.generated.h"

// ============================================================================
// IMPORTANT NOTE:
// ============================================================================
// Conditions are split into two intermediate classes to minimize interface
// pollution in the Blueprint/AngelScript editor and avoid polymorphic calls:
//   - UCk_SmCondition_Polled        → Evaluate() / DoEvaluate()
//   - UCk_SmCondition_EventDriven   → MarkSatisfied() / MarkUnsatisfied()
//
// EventDriven conditions don't use Evaluate(), so it won't appear in the
// editor. Polled conditions don't use MarkSatisfied/MarkUnsatisfied.
// Inherit from the appropriate intermediate class, not from this base.
// ============================================================================

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_EntityScript);

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
    // CONDITION LIFECYCLE (Enter/Exit)
    // ================================================================================================================
    //
    // EnterCondition fires from BeginPlay() once the condition entity is fully constructed.
    // EventDriven conditions typically bind external delegates here (and unbind in ExitCondition).
    // ExitCondition is called by FProcessor_SmCondition_Exit (EndPlay group, RunAfter SmTransition_Exit)
    // when FTag_SmCondition_PendingExit is present. The tag is cascaded from FProcessor_SmTransition_Exit,
    // which is in turn cascaded from FProcessor_SmState_Exit. The ECS lifecycle guarantees one-shot.

public:
    virtual auto
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    virtual auto
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Condition",
        DisplayName = "Enter Condition")
    void
    DoEnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Condition",
        DisplayName = "Exit Condition")
    void
    DoExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext);

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Get Owning StateMachine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    FCk_Handle_StateMachine
    Get_OwningStateMachine() const;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Get Parent Transition",
        meta = (CompactNodeTitle = "ParentTransition", HideSelfPin = true))
    FCk_Handle_SmTransition
    Get_ParentTransition() const;

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Get StateMachine Context",
        meta = (CompactNodeTitle = "Context", HideSelfPin = true))
    FCk_Handle
    Get_StateMachineContext() const;

    // ================================================================================================================
    // TAG
    // ================================================================================================================

public:
    auto
    Get_ConditionTag() const -> FGameplayTag;

    static auto
    Get_ConditionTagForClass(
        TSubclassOf<UCk_SmCondition_EntityScript> InClass) -> FGameplayTag;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition",
        meta = (AllowPrivateAccess = true))
    bool _NegateResult = false;

public:
    CK_PROPERTY_GET(_NegateResult);
};

// --------------------------------------------------------------------------------------------------------------------
