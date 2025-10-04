#pragma once

#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkDynamic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FScriptStructWildcard;
struct FAngelscriptAnyStructParameter;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_DynamicFragment_ForEachEntity, FCk_Handle, InHandle, UPARAM(ref) FInstancedStruct&,
    InFragment);

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
              DisplayName="[Ck][DynamicFragment] Add Fragment (Type Unsafe)")
    static FCk_Handle
    Add_Fragment(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InStructData);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] AddOrGet Fragment (Type Unsafe)",
              meta=(DeterminesOutputType="InStructType"))
    static UPARAM(ref) FInstancedStruct&
    AddOrGet_Fragment_TypeUnsafe(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Remove")
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Try Remove")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Get Fragment (Type Unsafe)")
    static UPARAM(ref) FInstancedStruct&
    Get_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Has Fragment")
    static bool
    Has_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Fragment",
              meta=(KeyWords = "get,all,fragments"))
    static void
    ForEach_EntityWithFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

public:
    static auto
    Get_StorageId(
        const UScriptStruct* InStructType) -> entt::id_type;

public:
    #if WITH_ANGELSCRIPT_CK
    static auto
    AddOrGet_Fragment(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FScriptStructWildcard&;

    static auto
    Get_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType)
    -> FScriptStructWildcard&;
    #endif
};

// --------------------------------------------------------------------------------------------------------------------
