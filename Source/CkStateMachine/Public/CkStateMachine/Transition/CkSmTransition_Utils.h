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
    MarkTransitionAs_StartEvaluating(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    static auto 
    MarkTransitionAs_EvaluationPassed(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    static auto 
    MarkTransitionAs_EvaluationFailed(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    static auto 
    MarkTransitionAs_ReadyToTransition(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

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
};

// --------------------------------------------------------------------------------------------------------------------
