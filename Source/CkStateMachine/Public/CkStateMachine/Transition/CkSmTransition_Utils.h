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

    // Adds FTag_SmTransition_PendingExit and requests entity destruction. Called by
    // FProcessor_SmState_Exit when cascading exit. FProcessor_SmTransition_Exit (EndPlay group,
    // RunAfter SmTask_Exit) picks up the tag and cascades exit to conditions.
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

    // Resets a failed transition to Undetermined and unpauses its polled conditions
    // so they can re-evaluate on the next cycle. Event-driven conditions are left
    // unchanged — they re-trigger via Request_UpdateConditionResult when their event
    // fires again. Does NOT add FTag_SmTransition_Evaluating.
    static auto
    Request_ResetTransition(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // Internal — called by UCk_Utils_SmCondition_UE::Create when a Polled condition is added.
    // Removes FTag_SmTransition_FullyEventDriven and cascades to the parent state.
    static auto
    Request_MarkTransition_AsNotFullyEventDriven(
        FCk_Handle_SmTransition& InTransition) -> FCk_Handle_SmTransition;

    // Internal — called by UCk_Utils_SmCondition_UE::Create after a condition is connected
    // to the transition's record. Walks the transition's conditions; if all are EventDriven
    // AND at least one exists, marks the transition FullyEventDriven and asks the parent
    // state to recompute its own tag. Necessary because transitions default to NOT
    // FullyEventDriven at Create (to handle vacuous transitions correctly) — this restores
    // the optimization for transitions whose final composition is all-event-driven.
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
