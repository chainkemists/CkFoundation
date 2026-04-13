#pragma once

#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmTransition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmTransition"))
class CKSTATEMACHINE_API UCk_Utils_SmTransition_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmTransition_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmTransition);

    // ================================================================================================================
    // CREATION
    // ================================================================================================================

public:
    static auto
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> FCk_Handle_SmTransition;

    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    static auto
    Request_StartEvaluating(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    static auto
    Request_UpdateTransitionResult(
        FCk_Handle_SmTransition& InTransition,
        ECk_SmTransitionResult InResult) -> FCk_Handle_SmTransition;

    // Internal — called by UCk_Utils_SmCondition_UE::Create when a Polled condition is added.
    // Removes FTag_SmTransition_FullyEventDriven and cascades to the parent state.
    static auto
    Request_MarkTransition_AsNotFullyEventDriven(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Is Fully EventDriven")
    static bool
    Get_IsFullyEventDriven(
        const FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Get Evaluation Result")
    static ECk_SmTransitionResult
    Get_EvaluationResult(
        const FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Get Target State Class")
    static TSubclassOf<UCk_SmState_EntityScript>
    Get_TargetStateClass(
        const FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Get Owning State Machine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    static FCk_Handle_StateMachine
    Get_OwningStateMachine(
        const FCk_Handle_SmTransition& InTransition);
};

// --------------------------------------------------------------------------------------------------------------------
