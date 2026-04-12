#pragma once

#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSmTask_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmTask_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SmTask"))
class CKSTATEMACHINE_API UCk_Utils_SmTask_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SmTask_UE);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmTask);

public:
    // ================================================================================================================
    // CREATION
    // ================================================================================================================

    static FCk_Handle_SmTask
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass);

public:
    // ================================================================================================================
    // RESULT MARKING
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Mark Task As Succeeded")
    static FCk_Handle_SmTask
    MarkTaskAs_Succeeded(
        UPARAM(ref) FCk_Handle_SmTask& InTask);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Mark Task As Failed")
    static FCk_Handle_SmTask
    MarkTaskAs_Failed(
        UPARAM(ref) FCk_Handle_SmTask& InTask);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Mark Task As Running")
    static FCk_Handle_SmTask
    MarkTaskAs_Running(
        UPARAM(ref) FCk_Handle_SmTask& InTask);

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Get Last Result")
    static ECk_SmTaskResult
    Get_LastResult(
        const FCk_Handle_SmTask& InTask);
};

// --------------------------------------------------------------------------------------------------------------------
