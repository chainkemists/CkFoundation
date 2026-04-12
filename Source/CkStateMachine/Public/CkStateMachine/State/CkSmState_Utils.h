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
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    static auto
    Create(
        FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass) -> FCk_Handle_SmState;

    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // ================================================================================================================
    // EVALUATION MODEL
    // ================================================================================================================

    static auto
    Request_MarkState_AsTicking(
        FCk_Handle_SmState& InState) -> FCk_Handle_SmState;

    static auto 
    Request_MarkState_AsEventDriven(
        FCk_Handle_SmState& InState) -> FCk_Handle_SmState;

    // ================================================================================================================
    // DEBUG
    // ================================================================================================================

    static auto 
    TryCheckTransitionBreakpoint(
        FCk_Handle_StateMachine& InStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) -> void;

    static auto 
    TryRecordLastFiredTransition(
        FCk_Handle_StateMachine& InStateMachine,
        FCk_Handle_SmTransition& InTransition) -> void;

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

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
};

// --------------------------------------------------------------------------------------------------------------------
