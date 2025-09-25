#pragma once

#include "CkEntityTag/CkEntityTag_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEntityTag_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKENTITYTAG_API UCk_Utils_EntityTag_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityTag_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Add Tag (Using FName)")
    static FCk_Handle
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Add Tag (Using GameplayTag)")
    static FCk_Handle
    Add_UsingGameplayTag(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Has Tag (Using FName)")
    static bool
    Has(
        const FCk_Handle& InHandle,
        FName InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Has Tag (Using GameplayTag)")
    static bool
    Has_UsingGameplayTag(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Get Tags as GameplayTagContainer")
    static FGameplayTagContainer
    Get_AllTagsAsContainer(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Get All Tags (Debug Only)",
              meta = (DevelopmentOnly))
    static TArray<FName>
    Get_AllTags(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Request Remove (Using FName)")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FName InTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Request Remove (Using GameplayTag)")
    static ECk_SucceededFailed
    Request_TryRemove_UsingGameplayTag(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] For Each (Using FName)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate", KeyWords = "get,all,tags"))
    static TArray<FCk_Handle>
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FName InTag);
    static auto
    ForEach_Entity(
        const FCk_Handle& InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FName InTag,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] For Each (Using GameplayTag)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate", KeyWords = "get,all,tags"))
    static TArray<FCk_Handle>
    ForEach_Entity_UsingGameplayTag(
        const FCk_Handle& InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag);
    static auto
    ForEach_Entity_UsingGameplayTag(
        const FCk_Handle& InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityTag",
        DisplayName = "[Ck][EntityTag] Bind To OnTagUpdated")
    static FCk_Handle
    BindTo_OnTagUpdated(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTag_OnTagUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityTag",
        DisplayName = "[Ck][EntityTag] Unbind From OnTagUpdated")
    static FCk_Handle
    UnbindFrom_OnTagUpdated(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_EntityTag_OnTagUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityTag",
        DisplayName = "[Ck][EntityTag] Bind To OnGameplayTagUpdated")
    static FCk_Handle
    BindTo_OnGameplayTagUpdated(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTagContainer InRelevantTags,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_EntityTag_OnGameplayTagUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityTag",
        DisplayName = "[Ck][EntityTag] Unbind From OnGameplayTagUpdated")
    static FCk_Handle
    UnbindFrom_OnGameplayTagUpdated(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_EntityTag_OnGameplayTagUpdated& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------