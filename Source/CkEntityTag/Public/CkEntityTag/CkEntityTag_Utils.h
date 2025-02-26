#pragma once

#include "CkEntityTag/CkEntityTag_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkEntityTag_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKENTITYTAG_API UCk_Utils_EntityTag_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityTag_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Add Feature")
    static FCk_Handle
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        FCk_Fragment_EntityTag_ParamsData InParams);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] Request Remove")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityTag",
              DisplayName="[Ck][EntityTag] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate", KeyWords = "get,all,tags"))
    static TArray<FCk_Handle>
    ForEach_Entity(
        FCk_Handle InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Entity(
        FCk_Handle InAnyHandle,
        UPARAM(meta = (Categories = "EntityTag"))
        FGameplayTag InTag,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------