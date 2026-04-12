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

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmTransition);

public:
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    static FCk_Handle_SmTransition
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass,
        int32 InOrder);

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Mark Transition As Start Evaluating")
    static FCk_Handle_SmTransition
    MarkTransitionAs_StartEvaluating(
        UPARAM(ref) FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Mark Transition As Evaluation Passed")
    static FCk_Handle_SmTransition
    MarkTransitionAs_EvaluationPassed(
        UPARAM(ref) FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Mark Transition As Evaluation Failed")
    static FCk_Handle_SmTransition
    MarkTransitionAs_EvaluationFailed(
        UPARAM(ref) FCk_Handle_SmTransition& InTransition);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Mark Transition As Ready To Transition")
    static FCk_Handle_SmTransition
    MarkTransitionAs_ReadyToTransition(
        UPARAM(ref) FCk_Handle_SmTransition& InTransition);

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
