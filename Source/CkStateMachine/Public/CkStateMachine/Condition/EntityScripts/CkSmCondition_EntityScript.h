#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_EntityScript.generated.h"

// Inherit from UCk_SmCondition_Polled (Evaluate / DoEvaluate) or UCk_SmCondition_EventDriven
// (MarkSatisfied / MarkUnsatisfied), never from this base — the split keeps each mode's interface
// out of the other's Blueprint/AngelScript editor surface.

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_EntityScript);

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

    // See UCk_SmState_EntityScript::Get_EffectiveReplication for rationale. Always DoesNotReplicate —
    // conditions are rebuilt locally via the SM replay path, never replicated as net objects.
    auto
    Get_EffectiveReplication() const -> ECk_Replication override;

    // --------------------------------------------------------------------------------------------------------------------
    // EnterCondition fires from BeginPlay once the condition entity is fully constructed; EventDriven
    // conditions typically bind external delegates here and unbind in ExitCondition. ExitCondition is
    // driven by FProcessor_SmCondition_Exit off FTag_SmCondition_PendingExit, cascaded State→Transition.

public:
    virtual auto
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    virtual auto
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

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

    // --------------------------------------------------------------------------------------------------------------------

public:
    auto
    Get_ConditionTag() const -> FGameplayTag;

    static auto
    Get_ConditionTagForClass(
        TSubclassOf<UCk_SmCondition_EntityScript> InClass) -> FGameplayTag;

    // --------------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition",
        meta = (AllowPrivateAccess = true))
    bool _NegateResult = false;

public:
    CK_PROPERTY_GET(_NegateResult);
};

// --------------------------------------------------------------------------------------------------------------------
