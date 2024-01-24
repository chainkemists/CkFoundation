#pragma once

#include "CkCore/Validation/CkIsValid.h"

#include <CoreMinimal.h>
#include <UObject/WeakInterfacePtr.h>
#include <GameplayTagContainer.h>
#include <InputCoreTypes.h>
#include <Framework/Commands/InputChord.h>
#include <InstancedStruct.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TSubclassOf);
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

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_IncludePendingKill);
CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_NullptrOnly);

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(bool, ck::IsValid_Policy_Default, [=](bool InBool)
{
    return InBool;
});

CK_DEFINE_CUSTOM_IS_VALID(const wchar_t*, ck::IsValid_Policy_Default, [=](const wchar_t* InText)
{
    return InText != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(const FField*, ck::IsValid_Policy_Default, [=](const FField* InField)
{
    return InField != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(const UObject*, ck::IsValid_Policy_Default, [=](const UObject* InObj)
{
    return ::IsValid(InObj);
});

CK_DEFINE_CUSTOM_IS_VALID(const AActor*, ck::IsValid_Policy_IncludePendingKill, [=](const AActor* InActor)
{
    return InActor != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FName, ck::IsValid_Policy_Default, [=](FName InName)
{
    return InName != NAME_None;
});

CK_DEFINE_CUSTOM_IS_VALID(FSoftObjectPath, ck::IsValid_Policy_Default, [=](const FSoftObjectPath& InSoftObject)
{
    return InSoftObject.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FSoftObjectPtr, ck::IsValid_Policy_Default, [=](const FSoftObjectPtr& InSoftObjectPtr)
{
    return InSoftObjectPtr.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FKey, ck::IsValid_Policy_Default, [=](const FKey& InKey)
{
    return InKey.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FInputChord, ck::IsValid_Policy_Default, [=](const FInputChord& InInputChord)
{
    return InInputChord.IsValidChord();
});

CK_DEFINE_CUSTOM_IS_VALID(FGuid, ck::IsValid_Policy_Default, [=](FGuid InGuid)
{
    return InGuid.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FGameplayTag, ck::IsValid_Policy_Default, [=](FGameplayTag InGameplayTag)
{
    return InGameplayTag.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FGameplayTagContainer, ck::IsValid_Policy_Default, [=](FGameplayTagContainer InGameplayTagContainer)
{
    return InGameplayTagContainer.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FInstancedStruct, ck::IsValid_Policy_Default, [=](const FInstancedStruct& InInstancedStruct)
{
    return InInstancedStruct.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, T*, ck::IsValid_Policy_NullptrOnly, [=](const T* InObj)
{
    return InObj != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSubclassOf<T>, ck::IsValid_Policy_Default, [=](const TSubclassOf<T>& InObj)
{
    return ck::IsValid(InObj.Get(), ck::IsValid_Policy_NullptrOnly{});
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
    return InPtr.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_T(T, TScriptInterface<T>, ck::IsValid_Policy_Default, [=](const TScriptInterface<T>& InScriptInterface)
{
    return ck::IsValid(InScriptInterface.GetObject()) && InScriptInterface.GetInterface() != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakInterfacePtr<T>, ck::IsValid_Policy_Default, [=](const TWeakInterfacePtr<T>& InInterfacePtr)
{
    return InInterfacePtr.Get_IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakObjectPtr<T>, ck::IsValid_Policy_Default, [=](const TWeakObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get());
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TWeakObjectPtr<T>, ck::IsValid_Policy_NullptrOnly, [=](const TWeakObjectPtr<T>& InObj)
{
    return ck::IsValid(InObj.Get(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TOptional<T>, ck::IsValid_Policy_Default, [=](const TOptional<T>& InOptional)
{
    if constexpr(std::is_pointer_v<T>)
    {
        return InOptional.IsSet() && ck::IsValid(InOptional.GetValue(), ck::IsValid_Policy_NullptrOnly{});
    }
    else
    {
        return InOptional.IsSet();
    }
});

CK_DEFINE_CUSTOM_IS_VALID_T(T, TOptional<T>, ck::IsValid_Policy_NullptrOnly, [=](const TOptional<T>& InOptional)
{
    static_assert(std::is_pointer_v<T>, "Type T is NOT a pointer. IsValid_Policy_NullptrOnly is not applicable");
    return ck::IsValid(InOptional.GetValue(), ck::IsValid_Policy_NullptrOnly{});
});

// --------------------------------------------------------------------------------------------------------------------
