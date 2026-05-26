#pragma once

#include "CkEntityTag/Query/CkEntityTagQuery_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include <GameplayTagContainer.h>

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

public:
    // ---- FName factories ----
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Single)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Single(
        FName InTag);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Count)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Of(
        FName InTag,
        int32 InCount);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (All)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_All(
        FName InTag);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Single, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Single_WithEnsure(
        FName InTag,
        int32 InMaxAllowed);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Count, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Of_WithEnsure(
        FName InTag,
        int32 InCount,
        int32 InMaxAllowed);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (All, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_All_WithEnsure(
        FName InTag,
        int32 InMaxAllowed);

    // ---- GameplayTag factories ----
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Single, FromGameplayTag)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Single_FromGameplayTag(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Count, FromGameplayTag)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Of_FromGameplayTag(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag,
        int32 InCount);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (All, FromGameplayTag)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_All_FromGameplayTag(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Single, FromGameplayTag, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Single_FromGameplayTag_WithEnsure(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag,
        int32 InMaxAllowed);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (Count, FromGameplayTag, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_Of_FromGameplayTag_WithEnsure(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag,
        int32 InCount,
        int32 InMaxAllowed);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EntityTagQuery",
              DisplayName = "[Ck][EntityTagQuery] Make Requirement (All, FromGameplayTag, With Ensure)")
    static FCk_EntityTagQuery_Requirement
    Make_Requirement_All_FromGameplayTag_WithEnsure(
        UPARAM(meta = (Categories = "EntityTag")) FGameplayTag InTag,
        int32 InMaxAllowed);
};
