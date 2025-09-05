#pragma once

#include "CkObjective_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkObjective_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Objective"))
class CKOBJECTIVE_API UCk_Utils_Objective_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Objective_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Objective);

public:
    static FCk_Handle_Objective
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Objective_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Objective",
        DisplayName = "[Ck][Objective] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Objective
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Objective",
        DisplayName = "[Ck][Objective] Handle -> Objective Handle",
        meta = (CompactNodeTitle = "<AsObjective>", BlueprintAutocast))
    static FCk_Handle_Objective
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Objective Handle",
        Category = "Ck|Utils|Objective",
        meta = (CompactNodeTitle = "INVALID_ObjectiveHandle", Keywords = "make"))
    static FCk_Handle_Objective
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Request Start")
    static FCk_Handle_Objective
    Request_Start(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Start& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Request Complete")
    static FCk_Handle_Objective
    Request_Complete(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Complete& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Request Fail")
    static FCk_Handle_Objective
    Request_Fail(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Fail& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Get Status")
    static ECk_ObjectiveStatus
    Get_Status(
        const FCk_Handle_Objective& InObjective);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Objective& InObjective);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Get Display Name")
    static FText
    Get_DisplayName(
        const FCk_Handle_Objective& InObjective);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Get Description")
    static FText
    Get_Description(
        const FCk_Handle_Objective& InObjective);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Bind To OnStatusChanged")
    static FCk_Handle_Objective
    BindTo_OnStatusChanged(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_StatusChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Bind To OnCompleted")
    static FCk_Handle_Objective
    BindTo_OnCompleted(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_Completed& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Bind To OnFailed")
    static FCk_Handle_Objective
    BindTo_OnFailed(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_Failed& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Unbind From OnStatusChanged")
    static FCk_Handle_Objective
    UnbindFrom_OnStatusChanged(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_StatusChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Unbind From OnCompleted")
    static FCk_Handle_Objective
    UnbindFrom_OnCompleted(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_Completed& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Objective",
              DisplayName = "[Ck][Objective] Unbind From OnFailed")
    static FCk_Handle_Objective
    UnbindFrom_OnFailed(
        UPARAM(ref) FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_Failed& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------