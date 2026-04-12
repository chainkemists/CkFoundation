#pragma once

#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmCondition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmCondition"))
class CKSTATEMACHINE_API UCk_Utils_SmCondition_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmCondition_UE);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmCondition);

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Request Start Or Resume Evaluating")
    static FCk_Handle_SmCondition
    Request_StartOrResumeEvaluating(
        UPARAM(ref) FCk_Handle_SmCondition& InCondition);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Request Pause Evaluation")
    static FCk_Handle_SmCondition
    Request_PauseEvaluation(
        UPARAM(ref) FCk_Handle_SmCondition& InCondition);

    // ================================================================================================================
    // RESULT MARKING
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Mark Condition As Satisfied")
    static FCk_Handle_SmCondition
    MarkConditionAs_Satisfied(
        UPARAM(ref) FCk_Handle_SmCondition& InCondition);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Mark Condition As Unsatisfied")
    static FCk_Handle_SmCondition
    MarkConditionAs_Unsatisfied(
        UPARAM(ref) FCk_Handle_SmCondition& InCondition);

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Get Evaluation Result")
    static ECk_SmConditionResult
    Get_EvaluationResult(
        const FCk_Handle_SmCondition& InCondition);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Get Is Event Driven")
    static bool
    Get_IsEventDriven(
        const FCk_Handle_SmCondition& InCondition);
};

// --------------------------------------------------------------------------------------------------------------------
