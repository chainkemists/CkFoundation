#pragma once

#include "CkCore/Validation/CkIsValid.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <GameplayTagsManager.h>
#include <InputCoreTypes.h>
#include <InstancedStruct.h>
#include <NativeGameplayTags.h>
#include <Curves/CurveFloat.h>
#include <Engine/CurveTable.h>
#include <Engine/DataTable.h>
#include <Framework/Commands/InputChord.h>
#include <Iris/Core/NetObjectReference.h>
#include <Misc/NetworkGuid.h>
#include <UObject/StrongObjectPtr.h>
#include <UObject/WeakInterfacePtr.h>

#include <type_traits>

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

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(std::function);

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_IncludePendingKill);
CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_NullptrOnly);
CK_DEFINE_CUSTOM_IS_VALID_POLICY(IsValid_Policy_OptionalEngagedOnly);

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
    return ::IsValid(InObj) && NOT InObj->IsUnreachable();
});

CK_DEFINE_CUSTOM_IS_VALID(const UObject*, ck::IsValid_Policy_IncludePendingKill, [=](const UObject* InObj)
{
    return InObj != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FName, ck::IsValid_Policy_Default, [=](FName InName)
{
    return InName != NAME_None;
});

CK_DEFINE_CUSTOM_IS_VALID(FRuntimeFloatCurve, ck::IsValid_Policy_Default, [=](FRuntimeFloatCurve InRuntimeFloatCurve)
{
    return ck::IsValid(InRuntimeFloatCurve.GetRichCurveConst(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID(FCurveTableRowHandle, ck::IsValid_Policy_Default, [=](const FCurveTableRowHandle& InCurveTableRowHandle)
{
    if (InCurveTableRowHandle.IsNull())
    { return false; }

    return InCurveTableRowHandle.CurveTable->FindCurveUnchecked(InCurveTableRowHandle.RowName) != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FDataTableRowHandle, ck::IsValid_Policy_Default, [=](const FDataTableRowHandle& InDataTableRowHandle)
{
    if (InDataTableRowHandle.IsNull())
    { return false; }

    return InDataTableRowHandle.DataTable->FindRowUnchecked(InDataTableRowHandle.RowName) != nullptr;
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

CK_DEFINE_CUSTOM_IS_VALID(FGuid, ck::IsValid_Policy_Default, [=](const FGuid& InGuid)
{
    return InGuid.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FNetworkGUID, ck::IsValid_Policy_Default, [=](const FNetworkGUID& InGuid)
{
    return InGuid.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FNativeGameplayTag, ck::IsValid_Policy_Default, [=](const FNativeGameplayTag& InGameplayTag)
{
    return ck::IsValid(InGameplayTag.GetTag());
});

#if CK_SKIP_VALIDATE_GAMEPLAYTAG_STALENESS
CK_DEFINE_CUSTOM_IS_VALID(FGameplayTag, ck::IsValid_Policy_Default, [=](const FGameplayTag& InGameplayTag)
{
    return InGameplayTag.IsValid();
});
#else
CK_DEFINE_CUSTOM_IS_VALID(FGameplayTag, ck::IsValid_Policy_Default, [=](const FGameplayTag& InGameplayTag)
{
    constexpr auto ErrorIfNotFound = false;
    return UGameplayTagsManager::Get().RequestGameplayTag(InGameplayTag.GetTagName(), ErrorIfNotFound).IsValid();
});
#endif

CK_DEFINE_CUSTOM_IS_VALID(FGameplayTagContainer, ck::IsValid_Policy_Default, [=](const FGameplayTagContainer& InGameplayTagContainer)
{
    return InGameplayTagContainer.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FInstancedStruct, ck::IsValid_Policy_Default, [=](const FInstancedStruct& InInstancedStruct)
{
    return InInstancedStruct.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FBox, ck::IsValid_Policy_Default, [=](const FBox& InBox3D)
{
    return static_cast<bool>(InBox3D.IsValid);
});

CK_DEFINE_CUSTOM_IS_VALID(FBox2D, ck::IsValid_Policy_Default, [=](const FBox2D& InBox2D)
{
    return static_cast<bool>(InBox2D.bIsValid);
});

CK_DEFINE_CUSTOM_IS_VALID(UE::Net::FNetObjectReference, ck::IsValid_Policy_Default, [=](const UE::Net::FNetObjectReference& InNetObjectReference)
{
    return InNetObjectReference.IsValid();
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

CK_DEFINE_CUSTOM_IS_VALID_T(T, TSoftClassPtr<T>, ck::IsValid_Policy_Default, [=](const TSoftClassPtr<T>& InObj)
{
    return InObj.IsValid();
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

CK_DEFINE_CUSTOM_IS_VALID_T(T, TInstancedStruct<T>, ck::IsValid_Policy_Default, [=](const TInstancedStruct<T>& InInstancedStruct)
{
    return InInstancedStruct.IsValid();
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
