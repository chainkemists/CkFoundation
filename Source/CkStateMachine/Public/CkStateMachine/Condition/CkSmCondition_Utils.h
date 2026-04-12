#pragma once

#include "CkStateMachine/Condition/CkSmCondition_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmCondition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmCondition"))
class CKSTATEMACHINE_API UCk_Utils_SmCondition_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmCondition_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmCondition);

    // ================================================================================================================
    // CREATION
    // ================================================================================================================
    
public:
    static FCk_Handle_SmCondition
    Create(
        FCk_Handle_SmTransition& InOwnerTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    static FCk_Handle_SmCondition
    Request_StartOrResumeEvaluating(
        FCk_Handle_SmCondition& InCondition);

    static FCk_Handle_SmCondition
    Request_PauseEvaluation(
        FCk_Handle_SmCondition& InCondition);

    // ================================================================================================================
    // RESULT MARKING
    // ================================================================================================================

    static FCk_Handle_SmCondition
    MarkConditionAs_Satisfied(
        FCk_Handle_SmCondition& InCondition);

    static FCk_Handle_SmCondition
    MarkConditionAs_Unsatisfied(
        FCk_Handle_SmCondition& InCondition);

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
        DisplayName = "[Ck][SmCondition] Get Condition Mode")
    static ECk_SmConditionMode
    Get_ConditionMode(
        const FCk_Handle_SmCondition& InCondition);
};

// --------------------------------------------------------------------------------------------------------------------
