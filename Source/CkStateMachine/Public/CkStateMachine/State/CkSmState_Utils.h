#pragma once

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

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

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    static FCk_Handle_SmState
    Create(
        FCk_Handle_StateMachine& InOwnerStateMachine,
        TSubclassOf<UCk_SmState_EntityScript> InStateClass);

public:
    // ================================================================================================================
    // EVALUATION MODEL
    // ================================================================================================================

    static FCk_Handle_SmState
    MarkStateAs_Ticking(
        FCk_Handle_SmState& InState);

    static FCk_Handle_SmState
    MarkStateAs_EventDriven(
        FCk_Handle_SmState& InState);

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
