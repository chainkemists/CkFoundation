#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkObjectiveOwner_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ObjectiveOwner_HandleRequests;
}

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ObjectiveOwner"))
class CKOBJECTIVE_API UCk_Utils_ObjectiveOwner_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ObjectiveOwner_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ObjectiveOwner);

public:
    friend class ck::FProcessor_ObjectiveOwner_HandleRequests;

private:
    struct RecordOfObjectives_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfObjectives> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Add Feature")
    static FCk_Handle_ObjectiveOwner
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_ObjectiveOwner_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ObjectiveOwner",
        DisplayName = "[Ck][ObjectiveOwner] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ObjectiveOwner
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ObjectiveOwner",
        DisplayName = "[Ck][ObjectiveOwner] Handle -> ObjectiveOwner Handle",
        meta = (CompactNodeTitle = "<AsObjectiveOwner>", BlueprintAutocast))
    static FCk_Handle_ObjectiveOwner
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid ObjectiveOwner Handle",
        Category = "Ck|Utils|ObjectiveOwner",
        meta = (CompactNodeTitle = "INVALID_ObjectiveOwnerHandle", Keywords = "make"))
    static FCk_Handle_ObjectiveOwner
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Request Add Objective")
    static FCk_Handle_ObjectiveOwner
    Request_AddObjective(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Request_ObjectiveOwner_AddObjective& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Request Remove Objective")
    static FCk_Handle_ObjectiveOwner
    Request_RemoveObjective(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName="[Ck][ObjectiveOwner] Try Get Objective")
    static FCk_Handle_Objective
    TryGet_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        UPARAM(meta = (Categories = "Objective")) FGameplayTag InObjectiveName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName="[Ck][ObjectiveOwner] ForEach Objective")
    static TArray<FCk_Handle_Objective>
    ForEach_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner);

    static auto
    ForEach_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        const TFunction<void(FCk_Handle_Objective)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName="[Ck][ObjectiveOwner] ForEach Objective (With Status)")
    static TArray<FCk_Handle_Objective>
    ForEach_Objective_WithStatus(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        ECk_ObjectiveStatus InObjectiveStatus);

    static auto
    ForEach_Objective_WithStatus(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        ECk_ObjectiveStatus InObjectiveStatus,
        const TFunction<void(FCk_Handle_Objective)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Bind To OnObjectiveAdded")
    static FCk_Handle_ObjectiveOwner
    BindTo_OnObjectiveAdded(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Bind To OnObjectiveRemoved")
    static FCk_Handle_ObjectiveOwner
    BindTo_OnObjectiveRemoved(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveRemoved& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Bind To OnObjectiveStatusChanged")
    static FCk_Handle_ObjectiveOwner
    BindTo_OnObjectiveStatusChanged(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Unbind From OnObjectiveAdded")
    static FCk_Handle_ObjectiveOwner
    UnbindFrom_OnObjectiveAdded(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveAdded& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Unbind From OnObjectiveRemoved")
    static FCk_Handle_ObjectiveOwner
    UnbindFrom_OnObjectiveRemoved(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveRemoved& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ObjectiveOwner",
              DisplayName = "[Ck][ObjectiveOwner] Unbind From OnObjectiveStatusChanged")
    static FCk_Handle_ObjectiveOwner
    UnbindFrom_OnObjectiveStatusChanged(
        UPARAM(ref) FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------