#pragma once

#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmState_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmState"))
class CKSTATEMACHINE_API UCk_Utils_SmState_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmState_UE);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmState);

public:
    // ================================================================================================================
    // EVALUATION MODEL
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Mark State As Ticking")
    static FCk_Handle_SmState
    MarkStateAs_Ticking(
        UPARAM(ref) FCk_Handle_SmState& InState);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmState",
        DisplayName = "[Ck][SmState] Mark State As Event Driven")
    static FCk_Handle_SmState
    MarkStateAs_EventDriven(
        UPARAM(ref) FCk_Handle_SmState& InState);

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
