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
    static auto 
    Create(
        FCk_Handle_SmTransition& InOwnerTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass) -> FCk_Handle_SmCondition;

    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // ================================================================================================================
    // EVALUATION LIFECYCLE
    // ================================================================================================================

    static auto
    Request_StartOrResumeEvaluating(
        FCk_Handle_SmCondition& InCondition) -> FCk_Handle_SmCondition;

    static auto
    Request_PauseEvaluation(
        FCk_Handle_SmCondition& InCondition) -> FCk_Handle_SmCondition;

    static auto
    Request_ResetCondition(
        FCk_Handle_SmCondition& InCondition) -> FCk_Handle_SmCondition;

    // ================================================================================================================
    // RESULT MARKING
    // ================================================================================================================

    static auto
    Request_UpdateConditionResult(
        FCk_Handle_SmCondition& InCondition,
        ECk_SmConditionResult InResult) -> FCk_Handle_SmCondition;

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

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Get Owning State Machine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    static FCk_Handle_StateMachine
    Get_OwningStateMachine(
        const FCk_Handle_SmCondition& InCondition);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmCondition",
        DisplayName = "[Ck][SmCondition] Get Script Class",
        meta = (CompactNodeTitle = "Condition Script", HideSelfPin = true))
    static TSubclassOf<UCk_SmCondition_EntityScript>
    Get_ScriptClass(
        const FCk_Handle_SmCondition& InCondition);
};

// --------------------------------------------------------------------------------------------------------------------
