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

    // --------------------------------------------------------------------------------------------------------------------

public:
    static auto
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> FCk_Handle_SmTransition;

    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // --------------------------------------------------------------------------------------------------------------------

    // FProcessor_SmTransition_Exit (EndPlay group, RunAfter SmTask_Exit) consumes the
    // PendingExit tag left here and cascades exit to the transition's conditions.
    static auto
    Request_Exit(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

public:
    // --------------------------------------------------------------------------------------------------------------------

    static auto
    Request_StartEvaluating(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    static auto
    Request_UpdateTransitionResult(
        FCk_Handle_SmTransition& InTransition,
        ECk_SmTransitionResult InResult) -> FCk_Handle_SmTransition;

    // Resets polled conditions only; event-driven ones keep their result and re-trigger via
    // Request_UpdateConditionResult. Does NOT add FTag_SmTransition_Evaluating.
    static auto
    Request_ResetTransition(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // Internal — called by UCk_Utils_SmCondition_UE::Create when a Polled condition is added.
    static auto
    Request_MarkTransition_AsNotFullyEventDriven(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // Internal — called by UCk_Utils_SmCondition_UE::Create after a condition is connected to
    // the transition's record; restores the tag transitions deliberately lack at Create.
    static auto
    Request_RecomputeFullyEventDrivenStatus(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // --------------------------------------------------------------------------------------------------------------------

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
