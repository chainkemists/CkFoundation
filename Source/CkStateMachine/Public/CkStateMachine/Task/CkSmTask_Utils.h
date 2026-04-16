#pragma once

#include "CkStateMachine/Task/CkSmTask_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

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
    Request_UpdateTaskResult(
        FCk_Handle_SmTask& InTask,
        ECk_SmTaskResult InResult) -> FCk_Handle_SmTask;

    // ================================================================================================================
    // QUERIES
    // ================================================================================================================

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Get Last Result")
    static ECk_SmTaskResult
    Get_LastResult(
        const FCk_Handle_SmTask& InTask);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Get Owning State Machine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    static FCk_Handle_StateMachine
    Get_OwningStateMachine(
        const FCk_Handle_SmTask& InTask);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Get Script Class",
        meta = (CompactNodeTitle = "Task Script", HideSelfPin = true))
    static TSubclassOf<UCk_SmTask_EntityScript>
    Get_ScriptClass(
        const FCk_Handle_SmTask& InTask);

    // ================================================================================================================
    // SIGNALS
    // ================================================================================================================

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Bind To OnSubSmConstructed")
    static FCk_Handle_SmTask
    BindTo_OnSubSmConstructed(
        UPARAM(ref) FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnSubSmConstructed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::Unbind);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SmTask",
        DisplayName = "[Ck][SmTask] Unbind From OnSubSmConstructed")
    static FCk_Handle_SmTask
    UnbindFrom_OnSubSmConstructed(
        UPARAM(ref) FCk_Handle_SmTask& InTask,
        const FCk_Delegate_SmTask_OnSubSmConstructed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
