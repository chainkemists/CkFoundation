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
    static FCk_Handle_SmTransition
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTransition",
        DisplayName = "[Ck][SmTransition] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    static FCk_Handle_SmTransition
    Request_StartOrResumeEvaluating(
        FCk_Handle_SmTransition& InTransition);

    static FCk_Handle_SmTransition
    MarkTransitionAs_StartEvaluating(
        FCk_Handle_SmTransition& InTransition);

    static FCk_Handle_SmTransition
    MarkTransitionAs_EvaluationPassed(
        FCk_Handle_SmTransition& InTransition);

    static FCk_Handle_SmTransition
    MarkTransitionAs_EvaluationFailed(
        FCk_Handle_SmTransition& InTransition);

    static FCk_Handle_SmTransition
    MarkTransitionAs_ReadyToTransition(
        FCk_Handle_SmTransition& InTransition);

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
