#pragma once

#include "CkEntityTag/CkEntityTag_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

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
              DisplayName="[Ck][EntityTag] Add Feature (Using FName)")
    static FCk_Handle
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Add Feature (Using GameplayTag)")
    static FCk_Handle
    Add_UsingGameplayTag(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag _Tag);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Request Remove")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FName InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Try Get Tag")
    static FName
    TryGet_Tag(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] For Each",
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
};

// --------------------------------------------------------------------------------------------------------------------