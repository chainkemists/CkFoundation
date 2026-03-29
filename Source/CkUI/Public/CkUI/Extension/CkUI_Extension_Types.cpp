// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Extension/CkUI_Extension_Types.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Extension/CkUI_Extension_Subsystem.h"

#include <Blueprint/UserWidget.h>

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_Extension
// --------------------------------------------------------------------------------------------------------------------

FCk_UI_Extension::FCk_UI_Extension(
    FGameplayTag InExtensionPointTag,
    UUserWidget* InWidgetInstance)
    : _ExtensionPointTag(InExtensionPointTag)
    , _WidgetClass(ck::IsValid(InWidgetInstance) ? InWidgetInstance->GetClass() : nullptr)
    , _WidgetInstance(InWidgetInstance)
{
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionPoint
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UI_ExtensionPoint::
    DoesExtensionPassContract(
        const FCk_UI_Extension* InExtension)
        -> bool
{
    if (ck::Is_NOT_Valid(InExtension, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return ck::IsValid(InExtension->Get_WidgetClass());
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionPointHandle
// --------------------------------------------------------------------------------------------------------------------

FCk_UI_ExtensionPointHandle::FCk_UI_ExtensionPointHandle(
    UCk_UI_Extension_Subsystem_UE* InSource,
    const TSharedPtr<FCk_UI_ExtensionPoint>& InDataPtr)
    : _ExtensionSource(InSource)
    , _DataPtr(InDataPtr)
{
}

auto
    FCk_UI_ExtensionPointHandle::
    Unregister()
    -> void
{
    if (auto* Source = _ExtensionSource.Get();
        ck::IsValid(Source))
    {
        Source->UnregisterExtensionPoint(*this);
    }
}

auto
    FCk_UI_ExtensionPointHandle::
    IsValid() const
    -> bool
{
    return _DataPtr.IsValid();
}

auto
    FCk_UI_ExtensionPointHandle::
    operator==(
        const ThisType& Other) const
    -> bool
{
    return _DataPtr == Other._DataPtr;
}

auto
    GetTypeHash(
        const FCk_UI_ExtensionPointHandle& InHandle)
    -> uint32
{
    return PointerHash(InHandle.Get_DataPtr().Get());
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_ExtensionHandle
// --------------------------------------------------------------------------------------------------------------------

FCk_UI_ExtensionHandle::FCk_UI_ExtensionHandle(
    UCk_UI_Extension_Subsystem_UE* InSource,
    const TSharedPtr<FCk_UI_Extension>& InDataPtr)
    : _ExtensionSource(InSource)
    , _DataPtr(InDataPtr)
{
}

auto
    FCk_UI_ExtensionHandle::
    Unregister()
    -> void
{
    if (auto* Source = _ExtensionSource.Get();
        ck::IsValid(Source))
    {
        Source->UnregisterExtension(*this);
    }
}

auto
    FCk_UI_ExtensionHandle::
    IsValid() const
    -> bool
{
    return _DataPtr.IsValid();
}

auto
    FCk_UI_ExtensionHandle::
    operator==(
        const ThisType& Other) const
    -> bool
{
    return _DataPtr == Other._DataPtr;
}

auto
    GetTypeHash(
        const FCk_UI_ExtensionHandle& InHandle)
    -> uint32
{
    return PointerHash(InHandle.Get_DataPtr().Get());
}

// --------------------------------------------------------------------------------------------------------------------