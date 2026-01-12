// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_LayerStack_ContextProvider.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Context/CkUI_Context_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_ContextReceiver
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    OnContextInjected_Implementation(
        const FCk_UI_Context& InContext)
    -> void
{
    _StoredContext = InContext;
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    OnContextCleared_Implementation()
    -> void
{
    _StoredContext = FCk_UI_Context{};
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Get_ShouldInheritContextFromParent_Implementation() const
    -> bool
{
    return _ShouldInheritFromParent;
}

// --------------------------------------------------------------------------------------------------------------------
// Context Provider API
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Get_LayerContext() const
    -> FCk_UI_Context
{
    return _StoredContext;
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Has_ValidLayerContext() const
    -> bool
{
    return _StoredContext.IsValid();
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Set_InjectionMode(
        ECk_UI_ContextInjectionMode InMode)
    -> void
{
    _InjectionMode = InMode;
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Get_InjectionMode() const
    -> ECk_UI_ContextInjectionMode
{
    return _InjectionMode;
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Set_ShouldInheritFromParent(
        bool InShouldInherit)
    -> void
{
    _ShouldInheritFromParent = InShouldInherit;
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_UI_LayerStack_UE Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    OnPostWidgetPush(
        UCommonActivatableWidget* InWidget)
    -> void
{
    Super::OnPostWidgetPush(InWidget);

    if (NOT ShouldInjectContextToWidget(InWidget))
    { return; }

    UCk_Utils_UI_Context_UE::InjectContext(InWidget, _StoredContext);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    ShouldInjectContextToWidget(
        UCommonActivatableWidget* InWidget) const
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    if (NOT Has_ValidLayerContext())
    { return false; }

    switch (_InjectionMode)
    {
        case ECk_UI_ContextInjectionMode::Never:
        {
            return false;
        }
        case ECk_UI_ContextInjectionMode::OnlyIfMissing:
        {
            return NOT UCk_Utils_UI_Context_UE::Has_ValidContextForWidget(InWidget);
        }
        case ECk_UI_ContextInjectionMode::Always:
        {
            return true;
        }
        default:
        {
            CK_INVALID_ENUM(_InjectionMode);
            break;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------