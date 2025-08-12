#pragma once

#include "CkCore/Validation/CkIsValid.h"

#include <InstancedStruct.h>

#include <type_traits>
#include <functional>

// --------------------------------------------------------------------------------------------------------------------

class FNativeGameplayTag;
struct FGameplayTag;
struct FGameplayTagContainer;
struct FRuntimeFloatCurve;
struct FCurveTableRowHandle;
struct FDataTableRowHandle;
struct FKey;
struct FInputChord;
class FNetworkGUID;

template<class T>
struct TWeakInterfacePtr;

namespace UE::Net
{
    class FNetObjectReference;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSubclassOf);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSoftClassPtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TScriptInterface);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TWeakInterfacePtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TWeakObjectPtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TWeakPtr, ESPMode::ThreadSafe);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TWeakPtr, ESPMode::NotThreadSafe);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TOptional);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSharedPtr, ESPMode::ThreadSafe);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSharedPtr, ESPMode::NotThreadSafe);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TUniquePtr);

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TObjectPtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSoftObjectPtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TStrongObjectPtr);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TInstancedStruct);
CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TPimplPtr);

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(std::function);

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_IncludePendingKill);
CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_NullptrOnly);
CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_OptionalEngagedOnly);

// --------------------------------------------------------------------------------------------------------------------

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API, bool, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID_CONST_PTR(CKCORE_API,wchar_t, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID_CONST_PTR(CKCORE_API,FField, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID_CONST_PTR(CKCORE_API,UObject, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID_CONST_PTR(CKCORE_API,UObject, IsValid_Policy_IncludePendingKill);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FName, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FRuntimeFloatCurve, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FCurveTableRowHandle, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FDataTableRowHandle, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FSoftObjectPath, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FSoftObjectPtr, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FKey, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FInputChord, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FGuid, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FNetworkGUID, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FNativeGameplayTag, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FGameplayTag, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FGameplayTagContainer, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FInstancedStruct, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FBox, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID(CKCORE_API,FBox2D, IsValid_Policy_Default);

CK_DECLARE_CUSTOM_IS_VALID_NAMESPACE(CKCORE_API, UE::Net, FNetObjectReference, IsValid_Policy_Default);

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, T*, ck::IsValid_Policy_NullptrOnly, [=](const T* InObj)
{
    return InObj != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSubclassOf<T>, ck::IsValid_Policy_Default, [=](const TSubclassOf<T>& InObj)
{
    return ck::IsValid(InObj.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSoftClassPtr<T>, ck::IsValid_Policy_Default, [=](const TSoftClassPtr<T>& InPtr)
{
    return NOT InPtr.IsNull();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSharedPtr<T>, ck::IsValid_Policy_Default, [=](const TSharedPtr<T>& InPtr)
{
    return ck::IsValid(InPtr.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSharedPtr<T COMMA ESPMode::NotThreadSafe>, ck::IsValid_Policy_Default, [=](const TSharedPtr<T, ESPMode::NotThreadSafe>& InPtr)
{
    return ck::IsValid(InPtr.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakPtr<T>, ck::IsValid_Policy_Default, [=](const TWeakPtr<T>& InPtr)
{
    return InPtr.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakPtr<T COMMA ESPMode::NotThreadSafe>, ck::IsValid_Policy_Default, [=](const TWeakPtr<T, ESPMode::NotThreadSafe>& InPtr)
{
    return InPtr.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TUniquePtr<T>, ck::IsValid_Policy_Default, [=](const TUniquePtr<T>& InPtr)
{
    return ck::IsValid(InPtr.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TObjectPtr<T>, ck::IsValid_Policy_Default, [=](const TObjectPtr<T>& InPtr)
{
    return ck::IsValid(InPtr.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSoftObjectPtr<T>, ck::IsValid_Policy_Default, [=](const TSoftObjectPtr<T>& InPtr)
{
    return NOT InPtr.IsNull();
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, TScriptInterface<T>, ck::IsValid_Policy_Default, [=](const TScriptInterface<T>& InScriptInterface)
{
    return ck::IsValid(InScriptInterface.GetObject()) && InScriptInterface.GetInterface() != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TInstancedStruct<T>, ck::IsValid_Policy_Default, [=](const TInstancedStruct<T>& InInstancedStruct)
{
    return InInstancedStruct.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TPimplPtr<T>, ck::IsValid_Policy_Default, [=](const TPimplPtr<T>& InPtr)
{
    return InPtr.IsValid() && ck::IsValid(InPtr.Get());
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TPimplPtr<T>, ck::IsValid_Policy_NullptrOnly, [=](const TPimplPtr<T>& InPtr)
{
    return InPtr.IsValid() && ck::IsValid(InPtr.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TStrongObjectPtr<T>, ck::IsValid_Policy_Default, [=](const TStrongObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get());
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TStrongObjectPtr<T>, ck::IsValid_Policy_NullptrOnly, [=](const TStrongObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakInterfacePtr<T>, ck::IsValid_Policy_Default, [=](const TWeakInterfacePtr<T>& InInterfacePtr)
{
    return InInterfacePtr.Get_IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakObjectPtr<T>, ck::IsValid_Policy_Default, [=](const TWeakObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get());
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakObjectPtr<T>, ck::IsValid_Policy_IncludePendingKill, [=](const TWeakObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get(true), ck::IsValid_Policy_IncludePendingKill{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakObjectPtr<T>, ck::IsValid_Policy_NullptrOnly, [=](const TWeakObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get(), ck::IsValid_Policy_NullptrOnly{});
});

// --------------------------------------------------------------------------------------------------------------------

namespace ck_details
{
    // This function is necessary for the TOptional Validator so that we can discard invalid
    // template specializations for types such as `float` which matches with both `bool` and
    // wchar_t* (not sure why). Forwarding the check to the following template function
    // allows us to discard the rest of the statements without having the fully checked
    // by the compiler.
    //
    // Ref: https://en.cppreference.com/w/cpp/language/if
    // search for "Outside a template, a discarded statement is fully checked. if constexpr is not a substitute for the #if preprocessing directive:"
    template <typename T>
    constexpr auto OptionalCheck(const TOptional<T>& InOptional)
    {
        if constexpr (std::is_enum_v<T> || std::is_integral_v<T> || std::is_arithmetic_v<T>)
        {
            return InOptional.IsSet();
        }
        else
        {
            if constexpr(ck::IsValid_Executor<T, ck::IsValid_Policy_Default>::value)
            {
                return InOptional.IsSet() && ck::IsValid(InOptional.GetValue());
            }
            else if constexpr(std::is_pointer_v<T>)
            {
                return InOptional.IsSet() && ck::IsValid(InOptional.GetValue(), ck::IsValid_Policy_NullptrOnly{});
            }
            else
            {
                return InOptional.IsSet();
            }
        }
    }
}

CK_DEFINE_CUSTOM_IS_VALID_T(T, TOptional<T>, ck::IsValid_Policy_Default, [=](const TOptional<T>& InOptional)
{
    return ck_details::OptionalCheck(InOptional);
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, TOptional<T>, ck::IsValid_Policy_NullptrOnly, [=](const TOptional<T>& InOptional)
{
    static_assert(std::is_pointer_v<T>, "Type T is NOT a pointer. IsValid_Policy_NullptrOnly is not applicable");
    return ck::IsValid(InOptional.GetValue(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TOptional<T>, ck::IsValid_Policy_OptionalEngagedOnly, [=](const TOptional<T>& InOptional)
{
    return InOptional.IsSet();
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, std::function<T>, ck::IsValid_Policy_Default, [=](const std::function<T>& InFunc)
{
    return InFunc == nullptr;
});

// --------------------------------------------------------------------------------------------------------------------
