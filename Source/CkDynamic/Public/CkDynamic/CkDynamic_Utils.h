#pragma once

#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include <AngelscriptAnyStructParameter.h>

#include "CkDynamic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_DynamicFragment_ForEachEntity, FCk_Handle, InHandle, UPARAM(ref) FAngelscriptAnyStructParameter&, InFragment);

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKDYNAMIC_API UCk_Utils_DynamicFragment_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DynamicFragment_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Add Fragment")
    static FCk_Handle
    Add_Fragment(
        UPARAM(ref) FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Remove")
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle& InHandle,
        UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Try Remove")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Get Fragment")
    static UPARAM(ref) FAngelscriptAnyStructParameter&
    Get_Fragment(
        const FCk_Handle& InHandle,
        UScriptStruct* InStructType);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Has Fragment")
    static bool
    Has_Fragment(
        const FCk_Handle& InHandle,
        UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Fragment",
              meta=(KeyWords = "get,all,fragments"))
    static void
    ForEach_EntityWithFragment(
        const FCk_Handle& InAnyHandle,
        UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

public:
    static auto
    Get_StorageId(
        UScriptStruct* InStructType)
        -> entt::id_type;
};

// --------------------------------------------------------------------------------------------------------------------
