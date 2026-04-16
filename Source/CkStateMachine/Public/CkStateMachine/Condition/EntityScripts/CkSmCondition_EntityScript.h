#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

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
