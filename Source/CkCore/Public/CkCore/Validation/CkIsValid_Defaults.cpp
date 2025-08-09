#include "CkIsValid_Defaults.h"

#include <CoreMinimal.h>
#include <GameplayTagContainer.h>
#include <GameplayTagsManager.h>
#include <NativeGameplayTags.h>

#include <Curves/CurveFloat.h>

#include <Engine/CurveTable.h>

#include <Framework/Commands/InputChord.h>

#include <Iris/Core/NetObjectReference.h>

#include <Misc/NetworkGuid.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(bool, IsValid_Policy_Default, [=](bool InBool)
{
    return InBool;
});

CK_DEFINE_CUSTOM_IS_VALID_CONST_PTR(wchar_t, IsValid_Policy_Default, [=](const wchar_t* InText)
{
    return InText != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_CONST_PTR(FField, IsValid_Policy_Default, [=](const FField* InField)
{
    return InField != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID_CONST_PTR(UObject, IsValid_Policy_Default, [=](const UObject* InObj)
{
    return ::IsValid(InObj) && NOT InObj->IsUnreachable();
});

CK_DEFINE_CUSTOM_IS_VALID_CONST_PTR(UObject, IsValid_Policy_IncludePendingKill, [=](const UObject* InObj)
{
    return InObj != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FName, IsValid_Policy_Default, [=](FName InName)
{
    return NOT InName.IsNone();
});

CK_DEFINE_CUSTOM_IS_VALID(FRuntimeFloatCurve, IsValid_Policy_Default, [=](const FRuntimeFloatCurve &InRuntimeFloatCurve)
{
    return ck::IsValid(InRuntimeFloatCurve.GetRichCurveConst(), ck::IsValid_Policy_NullptrOnly{});
});

CK_DEFINE_CUSTOM_IS_VALID(FCurveTableRowHandle, IsValid_Policy_Default, [=](const FCurveTableRowHandle& InCurveTableRowHandle)
{
    if (InCurveTableRowHandle.IsNull())
    { return false; }

    return InCurveTableRowHandle.CurveTable->FindCurveUnchecked(InCurveTableRowHandle.RowName) != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FDataTableRowHandle, IsValid_Policy_Default, [=](const FDataTableRowHandle& InDataTableRowHandle)
{
    if (InDataTableRowHandle.IsNull())
    { return false; }

    return InDataTableRowHandle.DataTable->FindRowUnchecked(InDataTableRowHandle.RowName) != nullptr;
});

CK_DEFINE_CUSTOM_IS_VALID(FSoftObjectPath, IsValid_Policy_Default, [=](const FSoftObjectPath& InSoftObject)
{
    return InSoftObject.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FSoftObjectPtr, IsValid_Policy_Default, [=](const FSoftObjectPtr& InSoftObjectPtr)
{
    return InSoftObjectPtr.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FKey, IsValid_Policy_Default, [=](const FKey& InKey)
{
    return InKey.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FInputChord, IsValid_Policy_Default, [=](const FInputChord& InInputChord)
{
    return InInputChord.IsValidChord();
});

CK_DEFINE_CUSTOM_IS_VALID(FGuid, IsValid_Policy_Default, [=](const FGuid& InGuid)
{
    return InGuid.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FNetworkGUID, IsValid_Policy_Default, [=](const FNetworkGUID& InGuid)
{
    return InGuid.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FNativeGameplayTag, IsValid_Policy_Default, [=](const FNativeGameplayTag& InGameplayTag)
{
    return ck::IsValid(InGameplayTag.GetTag());
});

#if CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION
CK_DEFINE_CUSTOM_IS_VALID(FGameplayTag, IsValid_Policy_Default, [=](const FGameplayTag& InGameplayTag)
{
    return InGameplayTag.IsValid();
});
#else
CK_DEFINE_CUSTOM_IS_VALID(FGameplayTag, IsValid_Policy_Default, [=](const FGameplayTag& InGameplayTag)
{
    constexpr auto ErrorIfNotFound = false;
    return UGameplayTagsManager::Get().RequestGameplayTag(InGameplayTag.GetTagName(), ErrorIfNotFound).IsValid();
});
#endif

CK_DEFINE_CUSTOM_IS_VALID(FGameplayTagContainer, IsValid_Policy_Default, [=](const FGameplayTagContainer& InGameplayTagContainer)
{
    return InGameplayTagContainer.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FInstancedStruct, IsValid_Policy_Default, [=](const FInstancedStruct& InInstancedStruct)
{
    return InInstancedStruct.IsValid();
});

CK_DEFINE_CUSTOM_IS_VALID(FBox, IsValid_Policy_Default, [=](const FBox& InBox3D)
{
    return static_cast<bool>(InBox3D.IsValid);
});

CK_DEFINE_CUSTOM_IS_VALID(FBox2D, IsValid_Policy_Default, [=](const FBox2D& InBox2D)
{
    return static_cast<bool>(InBox2D.bIsValid);
});

CK_DEFINE_CUSTOM_IS_VALID_NAMESPACE(UE::Net, FNetObjectReference, IsValid_Policy_Default, [=](const UE::Net::FNetObjectReference& InNetObjectReference)
{
    return InNetObjectReference.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------
