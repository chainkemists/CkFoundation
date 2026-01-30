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

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_DynamicFragment_ForEachEntity_OneFragment, FCk_Handle, InHandle, UPARAM(ref) FInstancedStruct&, InFragment);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCk_DynamicFragment_ForEachEntity_TwoFragments, FCk_Handle, InHandle, UPARAM(ref) FInstancedStruct&, InFragmentA, UPARAM(ref) FInstancedStruct&, InFragmentB);
DECLARE_DYNAMIC_DELEGATE_FourParams(FCk_DynamicFragment_ForEachEntity_ThreeFragments, FCk_Handle, InHandle, UPARAM(ref) FInstancedStruct&, InFragmentA, UPARAM(ref) FInstancedStruct&, InFragmentB, UPARAM(ref) FInstancedStruct&, InFragmentC);
DECLARE_DYNAMIC_DELEGATE_FiveParams(FCk_DynamicFragment_ForEachEntity_FourFragments, FCk_Handle, InHandle, UPARAM(ref) FInstancedStruct&, InFragmentA, UPARAM(ref) FInstancedStruct&, InFragmentB, UPARAM(ref) FInstancedStruct&, InFragmentC, UPARAM(ref) FInstancedStruct&, InFragmentD);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle"))
class CKDYNAMIC_API UCk_Utils_DynamicFragment_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DynamicFragment_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Add Fragment (Type Unsafe)",
              meta=(BlueprintInternalUseOnly = "true"))
    static FCk_Handle
    Add_Fragment(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InStructData);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Add or Get Fragment (Type Unsafe)",
              meta=(DeterminesOutputType="InStructType", BlueprintInternalUseOnly = "true"))
    static FInstancedStruct&
    AddOrGet_Fragment_TypeUnsafe(
        UPARAM(ref) FCk_Handle& InHandle,
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
              DisplayName="[Ck][DynamicFragment] Get Fragment (Type Unsafe)",
              meta=(BlueprintInternalUseOnly = "true"))
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
              DisplayName="[Ck][DynamicFragment] For Each Entity With One Fragment",
              meta=(KeyWords = "get,all,fragments,1"))
    static void
    ForEach_EntityWithOneFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity_OneFragment& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Two Fragments",
              meta=(KeyWords = "get,all,fragments,2"))
    static void
    ForEach_EntityWithTwoFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const FCk_DynamicFragment_ForEachEntity_TwoFragments& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Three Fragments",
              meta=(KeyWords = "get,all,fragments,3"))
    static void
    ForEach_EntityWithThreeFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const FCk_DynamicFragment_ForEachEntity_ThreeFragments& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Four Fragments",
              meta=(KeyWords = "get,all,fragments,4"))
    static void
    ForEach_EntityWithFourFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const UScriptStruct* InStructTypeD,
        const FCk_DynamicFragment_ForEachEntity_FourFragments& InDelegate,
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
        const UScriptStruct* InStructType) -> FScriptStructWildcard&;

    static auto
    Get_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType) -> FScriptStructWildcard&;
#endif

private:
    template<size_t N, typename T_Callback>
    static auto
    ForEachEntity_WithDynamicFragments(
        const FCk_Handle& InAnyHandle,
        const std::array<const UScriptStruct*, N>& InStructTypes,
        ECk_DestroyFilter InFilter,
        T_Callback&& InCallback) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
