#pragma once

#include "CkEntityTag/Query/CkEntityTagQuery_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEntityTagQuery_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EntityTagQuery"))
class CKENTITYTAG_API UCk_Utils_EntityTagQuery_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityTagQuery_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityTagQuery);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Add")
    static FCk_Handle_EntityTagQuery
    Add(
        UPARAM(ref) FCk_Handle& InOwner);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Has")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Request Add Requirement")
    static FCk_Handle_EntityTagQuery
    Request_AddRequirement(
        UPARAM(ref) FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_AddRequirement& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Request Remove Requirement")
    static FCk_Handle_EntityTagQuery
    Request_RemoveRequirement(
        UPARAM(ref) FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Request_EntityTagQuery_RemoveRequirement& InRequest);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Get Current Results")
    static TArray<FCk_EntityTagQuery_Result>
    Get_CurrentResults(
        const FCk_Handle_EntityTagQuery& InQuery);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Get Is Satisfied")
    static bool
    Get_IsSatisfied(
        const FCk_Handle_EntityTagQuery& InQuery);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Get All Requirements")
    static TArray<FCk_EntityTagQuery_Requirement>
    Get_AllRequirements(
        const FCk_Handle_EntityTagQuery& InQuery);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Bind To OnSatisfied")
    static FCk_Handle_EntityTagQuery
    BindTo_OnSatisfied(
        UPARAM(ref) FCk_Handle_EntityTagQuery& InQuery,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTagQuery_OnSatisfied& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Unbind From OnSatisfied")
    static FCk_Handle_EntityTagQuery
    UnbindFrom_OnSatisfied(
        UPARAM(ref) FCk_Handle_EntityTagQuery& InQuery,
        const FCk_Delegate_EntityTagQuery_OnSatisfied& InDelegate);
};
