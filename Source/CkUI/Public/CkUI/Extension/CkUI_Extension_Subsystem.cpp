// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Extension/CkUI_Extension_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/UserWidget.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Lifecycle
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    Deinitialize()
    -> void
{
    _ExtensionPointMap.Empty();
    _ExtensionMap.Empty();
    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
// Extension Point Registration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    RegisterExtensionPoint(
        FGameplayTag InExtensionPointTag,
        ECk_UI_ExtensionPointMatch InMatchType,
        FCk_Delegate_ExtensionPoint_OnExtend InCallback)
    -> FCk_UI_ExtensionPointHandle
{
    if (ck::Is_NOT_Valid(InExtensionPointTag))
    { return {}; }

    if (NOT InCallback.IsBound())
    { return {}; }

    auto& List = _ExtensionPointMap.FindOrAdd(InExtensionPointTag);
    auto Entry = MakeShared<FCk_UI_ExtensionPoint>(InExtensionPointTag, MoveTemp(InCallback));
    Entry->Set_MatchType(InMatchType);

    List.Add(Entry);
    DoNotifyExtensionPointOfExtensions(Entry);

    return FCk_UI_ExtensionPointHandle(this, Entry);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    UnregisterExtensionPoint(
        const FCk_UI_ExtensionPointHandle& InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InHandle))
    { return; }

    const auto& ExtensionPoint = InHandle.Get_DataPtr();
    auto* ListPtr = _ExtensionPointMap.Find(ExtensionPoint->Get_ExtensionPointTag());

    if (ck::Is_NOT_Valid(ListPtr, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    ListPtr->RemoveSwap(ExtensionPoint);

    if (ListPtr->IsEmpty())
    {
        _ExtensionPointMap.Remove(ExtensionPoint->Get_ExtensionPointTag());
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Extension Registration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    RegisterExtension(
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    if (ck::Is_NOT_Valid(InExtensionPointTag))
    { return {}; }

    if (ck::Is_NOT_Valid(InWidgetClass))
    { return {}; }

    auto& List = _ExtensionMap.FindOrAdd(InExtensionPointTag);
    auto Entry = MakeShared<FCk_UI_Extension>(InExtensionPointTag, InWidgetClass);
    Entry->Set_Priority(InPriority);

    List.Add(Entry);
    DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction::Added, Entry);

    return FCk_UI_ExtensionHandle(this, Entry);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    UnregisterExtension(
        const FCk_UI_ExtensionHandle& InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InHandle))
    { return; }

    auto Extension = InHandle.Get_DataPtr();
    auto* ListPtr = _ExtensionMap.Find(Extension->Get_ExtensionPointTag());

    if (ck::Is_NOT_Valid(ListPtr, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction::Removed, Extension);

    ListPtr->RemoveSwap(Extension);

    if (ListPtr->IsEmpty())
    {
        _ExtensionMap.Remove(Extension->Get_ExtensionPointTag());
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Blueprint Registration Functions
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    DoRegisterExtensionPoint(
        FGameplayTag InExtensionPointTag,
        ECk_UI_ExtensionPointMatch InMatchType,
        FCk_DynamicDelegate_ExtensionPoint_OnExtend InCallback)
    -> FCk_UI_ExtensionPointHandle
{
    auto NativeCallback = FCk_Delegate_ExtensionPoint_OnExtend::CreateWeakLambda(
        InCallback.GetUObject(),
        [InCallback](ECk_UI_ExtensionAction InAction, const FCk_UI_ExtensionRequest& InRequest)
        {
            InCallback.ExecuteIfBound(InAction, InRequest);
        });

    return RegisterExtensionPoint(InExtensionPointTag, InMatchType, NativeCallback);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    DoRegisterExtension(
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    return RegisterExtension(InExtensionPointTag, InWidgetClass, InPriority);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    DoNotifyExtensionPointOfExtensions(
        TSharedPtr<FCk_UI_ExtensionPoint> InExtensionPoint)
    -> void
{
    for (auto Tag = InExtensionPoint->Get_ExtensionPointTag(); Tag.IsValid(); Tag = Tag.RequestDirectParent())
    {
        const auto* ListPtr = _ExtensionMap.Find(Tag);

        if (ck::Is_NOT_Valid(ListPtr, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (InExtensionPoint->Get_MatchType() == ECk_UI_ExtensionPointMatch::ExactMatch)
            { break; }

            continue;
        }

        const auto ExtensionArray = *ListPtr;

        for (const auto& Extension : ExtensionArray)
        {
            if (NOT InExtensionPoint->DoesExtensionPassContract(Extension.Get()))
            { continue; }

            auto Request = DoCreateExtensionRequest(Extension);
            InExtensionPoint->Get_Callback().ExecuteIfBound(ECk_UI_ExtensionAction::Added, Request);
        }

        if (InExtensionPoint->Get_MatchType() == ECk_UI_ExtensionPointMatch::ExactMatch)
        { break; }
    }
}

auto
    UCk_UI_Extension_Subsystem_UE::
    DoNotifyExtensionPointsOfExtension(
        ECk_UI_ExtensionAction InAction,
        TSharedPtr<FCk_UI_Extension> InExtension)
    -> void
{
    auto IsOnInitialTag = true;

    for (auto Tag = InExtension->Get_ExtensionPointTag(); Tag.IsValid(); Tag = Tag.RequestDirectParent())
    {
        const auto* ListPtr = _ExtensionPointMap.Find(Tag);

        if (ck::Is_NOT_Valid(ListPtr, ck::IsValid_Policy_NullptrOnly{}))
        {
            IsOnInitialTag = false;
            continue;
        }

        const auto ExtensionPointArray = *ListPtr;

        for (const auto& ExtensionPoint : ExtensionPointArray)
        {
            const auto ShouldNotify = IsOnInitialTag ||
                (ExtensionPoint->Get_MatchType() == ECk_UI_ExtensionPointMatch::PartialMatch);

            if (NOT ShouldNotify)
            { continue; }

            if (NOT ExtensionPoint->DoesExtensionPassContract(InExtension.Get()))
            { continue; }

            auto Request = DoCreateExtensionRequest(InExtension);
            ExtensionPoint->Get_Callback().ExecuteIfBound(InAction, Request);
        }

        IsOnInitialTag = false;
    }
}

auto
    UCk_UI_Extension_Subsystem_UE::
    DoCreateExtensionRequest(
        const TSharedPtr<FCk_UI_Extension>& InExtension)
    -> FCk_UI_ExtensionRequest
{
    return FCk_UI_ExtensionRequest(
        FCk_UI_ExtensionHandle(this, InExtension),
        InExtension->Get_ExtensionPointTag(),
        InExtension->Get_WidgetClass(),
        InExtension->Get_Priority());
}

// --------------------------------------------------------------------------------------------------------------------