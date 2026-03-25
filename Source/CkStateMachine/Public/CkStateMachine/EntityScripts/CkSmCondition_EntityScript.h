#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmCondition_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_EntityScript);

    UCk_SmCondition_EntityScript()
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

    // ================================================================================================================
    // VIRTUAL METHODS (user overrides — polled conditions)
    // ================================================================================================================

public:
    virtual auto
    Evaluate() const -> bool;

    // ================================================================================================================
    // MARK SATISFIED / UNSATISFIED (event-driven conditions)
    // ================================================================================================================

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Mark Satisfied")
    void
    MarkSatisfied();

    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Mark Unsatisfied")
    void
    MarkUnsatisfied();

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Condition",
        DisplayName = "Evaluate")
    bool
    DoEvaluate(
        FCk_Handle InHandle) const;

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Get Owner StateMachine",
        meta = (CompactNodeTitle = "OwnerSM", HideSelfPin = true))
    FCk_Handle_StateMachine
    DoGet_OwnerStateMachine() const;

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Condition",
        DisplayName = "[Ck][SM] Get Game Entity",
        meta = (CompactNodeTitle = "GameEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_GameEntity() const;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition",
        meta = (AllowPrivateAccess = true))
    ECk_SmConditionMode _ConditionMode = ECk_SmConditionMode::Polled;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition",
        meta = (AllowPrivateAccess = true))
    ECk_SmConditionResetBehavior _ResetBehavior = ECk_SmConditionResetBehavior::ResetEveryFrame;

public:
    CK_PROPERTY_GET(_ConditionMode);
    CK_PROPERTY_GET(_ResetBehavior);
};

// --------------------------------------------------------------------------------------------------------------------
