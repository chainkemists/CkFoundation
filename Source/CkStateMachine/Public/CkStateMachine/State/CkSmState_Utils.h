#pragma once

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmState_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmCondition_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmState"))
class CKSTATEMACHINE_API UCk_Utils_SmState_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmState_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmState);

public:
    // --------------------------------------------------------------------------------------------------------------------

    static auto
    Create(
        FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> FCk_Handle_SmState;

    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // --------------------------------------------------------------------------------------------------------------------

    // Adds FTag_SmState_PendingExit and (when InScheduleDestroy) requests entity destruction.
    // InScheduleDestroy=false runs the exit cascade but leaves the entity alive, so
    // FProcessor_Sm_CommitPendingTransition can keep reading the previous state handle and destroy
    // it only after the new state is entered.
    static auto
    Request_Exit(
        FCk_Handle_SmState& InState,
        bool InScheduleDestroy = true) -> FCk_Handle_SmState;

    // True while FProcessor_SmState_Exit has not yet cleared FTag_SmState_PendingExit.
    static auto
    Get_IsPendingExit(
        const FCk_Handle_SmState& InState) -> bool;

    // --------------------------------------------------------------------------------------------------------------------

    // Called automatically by FProcessor_SmState_Update (non-event-driven states) and
    // FProcessor_SmTransition_Evaluate (fully event-driven states, on Pass/Fail).
    static auto
    Request_Evaluate(
        FCk_Handle_SmState& InState) -> FCk_Handle_SmState;

    // Internal — called by UCk_Utils_SmTransition_UE::Request_MarkTransition_AsNotFullyEventDriven.
    static auto
    Request_MarkState_AsNotFullyEventDriven(
        FCk_Handle_SmState& InState) -> FCk_Handle_SmState;

    // Internal — called by UCk_Utils_SmTransition_UE::Request_RecomputeFullyEventDrivenStatus after a
    // transition's tag changes. Marks the state FullyEventDriven iff no transition is polled (so
    // FProcessor_SmState_Update skips it for perf); a terminal state with zero transitions qualifies.
    static auto
    Request_RecomputeFullyEventDrivenStatus(
        FCk_Handle_SmState& InState) -> FCk_Handle_SmState;

    // --------------------------------------------------------------------------------------------------------------------

    static auto 
    TryCheckTransitionBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> void;

    static auto 
    TryRecordLastFiredTransition(
        FCk_Handle_StateMachine& InStateMachine,
        FCk_Handle_SmTransition& InTransition) -> void;

    // --------------------------------------------------------------------------------------------------------------------

    // Returns true while no polled condition has been added to any of this state's transitions.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Is Fully EventDriven")
    static bool
    Get_IsFullyEventDriven(
        const FCk_Handle_SmState& InState);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Get Is Ready To Transition")
    static bool
    Get_IsReadyToTransition(
        const FCk_Handle_SmState& InState);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Get Owning State Machine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    static FCk_Handle_StateMachine
    Get_OwningStateMachine(
        const FCk_Handle_SmState& InState);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Get Hierarchy")
    static TArray<FGameplayTag>
    Get_Hierarchy(
        const FCk_Handle_SmState& InState);

    // The actual instantiated class (post-override resolution) — what is really running.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Get Script Class",
        meta = (CompactNodeTitle = "State Script", HideSelfPin = true))
    static TSubclassOf<UCk_SmState_EntityScript>
    Get_ScriptClass(
        const FCk_Handle_SmState& InState);

    // The class the caller asked for before FFragment_Sm_StateOverrides remapped it — equal to
    // Get_ScriptClass when no override applied.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Get Requested Script Class",
        meta = (CompactNodeTitle = "Requested Script", HideSelfPin = true))
    static TSubclassOf<UCk_SmState_EntityScript>
    Get_RequestedScriptClass(
        const FCk_Handle_SmState& InState);

    // Walks FFragment_Sm_StateOverrides; returns the overriding class if any entry matches the
    // prospective hierarchy, otherwise InRequestedClass. Callers never pre-resolve.
    static auto
    Get_ResolvedStateClass(
        const FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InRequestedClass) -> TSubclassOf<UCk_SmState_EntityScript>;

    // --------------------------------------------------------------------------------------------------------------------

private:

    static auto
    DoBuildProspectiveHierarchy(
        const FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> TArray<FGameplayTag>;
};

// --------------------------------------------------------------------------------------------------------------------
