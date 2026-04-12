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
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SmTask);

    // ================================================================================================================
    // CREATION
    // ================================================================================================================

public:
    static auto
    Create(
        FCk_Handle_SmState& InOwnerState,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) -> FCk_Handle_SmTask;

    static auto 
    Has(
        const FCk_Handle& InHandle) -> bool;

public:
    // ================================================================================================================
    // RESULT MARKING
    // ================================================================================================================

    static auto
    MarkTaskAs_Succeeded(
        FCk_Handle_SmTask& InTask) -> FCk_Handle_SmTask;

    static auto
    MarkTaskAs_Failed(
        FCk_Handle_SmTask& InTask) -> FCk_Handle_SmTask;

    static auto
    MarkTaskAs_Running(
        FCk_Handle_SmTask& InTask) -> FCk_Handle_SmTask;

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
