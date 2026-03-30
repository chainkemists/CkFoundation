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
    _WidgetToExtensionMap.Empty();
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
    const auto Entry = MakeShared<FCk_UI_ExtensionPoint>(InExtensionPointTag, MoveTemp(InCallback));
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
    auto* FoundList = _ExtensionPointMap.Find(ExtensionPoint->Get_ExtensionPointTag());

    if (ck::Is_NOT_Valid(FoundList, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    FoundList->RemoveSwap(ExtensionPoint);

    if (FoundList->IsEmpty())
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
    const auto Entry = MakeShared<FCk_UI_Extension>(InExtensionPointTag, InWidgetClass);
    Entry->Set_Priority(InPriority);

    List.Add(Entry);
    DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction::Added, Entry);

    return FCk_UI_ExtensionHandle(this, Entry);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    RegisterExtension(
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    if (ck::Is_NOT_Valid(InExtensionPointTag))
    { return {}; }

    if (ck::Is_NOT_Valid(InWidgetInstance))
    { return {}; }

    auto& List = _ExtensionMap.FindOrAdd(InExtensionPointTag);
    const auto Entry = MakeShared<FCk_UI_Extension>(InExtensionPointTag, InWidgetInstance);
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

    const auto Extension = InHandle.Get_DataPtr();
    auto* FoundList = _ExtensionMap.Find(Extension->Get_ExtensionPointTag());

    if (ck::Is_NOT_Valid(FoundList, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction::Removed, Extension);

    FoundList->RemoveSwap(Extension);

    if (FoundList->IsEmpty())
    {
        _ExtensionMap.Remove(Extension->Get_ExtensionPointTag());
    }
}

auto
    UCk_UI_Extension_Subsystem_UE::
    TryUnregisterExtensionByWidget(
        UUserWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    auto Handle = FCk_UI_ExtensionHandle{};

    if (NOT _WidgetToExtensionMap.RemoveAndCopyValue(TWeakObjectPtr<UUserWidget>(InWidget), Handle))
    { return false; }

    UnregisterExtension(Handle);
    return true;
}

auto
    UCk_UI_Extension_Subsystem_UE::
    ClearExtensionsAtPoint(
        FGameplayTag InExtensionPointTag)
    -> void
{
    if (ck::Is_NOT_Valid(InExtensionPointTag))
    { return; }

    auto* FoundList = _ExtensionMap.Find(InExtensionPointTag);

    if (ck::Is_NOT_Valid(FoundList, ck::IsValid_Policy_NullptrOnly{}) || FoundList->IsEmpty())
    { return; }

    const auto ExtensionsCopy = *FoundList;

    for (const auto& Extension : ExtensionsCopy)
    {
        DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction::Removed, Extension);

        if (const auto& WidgetInstance = Extension->Get_WidgetInstance();
            ck::IsValid(WidgetInstance))
        {
            _WidgetToExtensionMap.Remove(TWeakObjectPtr<UUserWidget>(WidgetInstance.Get()));
        }
    }

    _ExtensionMap.Remove(InExtensionPointTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Widget <-> Extension Mapping
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    NotifyWidgetCreatedForExtension(
        UUserWidget* InWidget,
        const FCk_UI_ExtensionHandle& InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    _WidgetToExtensionMap.Add(TWeakObjectPtr<UUserWidget>(InWidget), InHandle);
}

auto
    UCk_UI_Extension_Subsystem_UE::
    NotifyWidgetRemovedForExtension(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    _WidgetToExtensionMap.Remove(TWeakObjectPtr<UUserWidget>(InWidget));
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
    const auto NativeCallback = FCk_Delegate_ExtensionPoint_OnExtend::CreateWeakLambda(
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

auto
    UCk_UI_Extension_Subsystem_UE::
    DoRegisterExtension_Instance(
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    return RegisterExtension(InExtensionPointTag, InWidgetInstance, InPriority);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Extension_Subsystem_UE::
    DoNotifyExtensionPointOfExtensions(
        const TSharedPtr<FCk_UI_ExtensionPoint>& InExtensionPoint)
    -> void
{
    for (auto Tag = InExtensionPoint->Get_ExtensionPointTag(); Tag.IsValid(); Tag = Tag.RequestDirectParent())
    {
        const auto* FoundList = _ExtensionMap.Find(Tag);

        if (ck::Is_NOT_Valid(FoundList, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (InExtensionPoint->Get_MatchType() == ECk_UI_ExtensionPointMatch::ExactMatch)
            { break; }

            continue;
        }

        for (const auto ExtensionArray = *FoundList;
            const auto& Extension : ExtensionArray)
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
        const TSharedPtr<FCk_UI_Extension>& InExtension)
    -> void
{
    auto IsOnInitialTag = true;

    for (auto Tag = InExtension->Get_ExtensionPointTag(); Tag.IsValid(); Tag = Tag.RequestDirectParent())
    {
        const auto* FoundList = _ExtensionPointMap.Find(Tag);

        if (ck::Is_NOT_Valid(FoundList, ck::IsValid_Policy_NullptrOnly{}))
        {
            IsOnInitialTag = false;
            continue;
        }

        for (const auto ExtensionPointArray = *FoundList;
            const auto& ExtensionPoint : ExtensionPointArray)
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
        InExtension->Get_WidgetInstance(),
        InExtension->Get_Priority());
}

// --------------------------------------------------------------------------------------------------------------------