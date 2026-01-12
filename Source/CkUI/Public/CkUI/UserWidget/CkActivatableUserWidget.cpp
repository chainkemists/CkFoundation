// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/UserWidget/CkActivatableUserWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Context/CkUI_Context_Utils.h"
#include "CkUI/UserWidget/CkUserWidget.h"

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_ContextReceiver Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    OnContextInjected_Implementation(
        const FCk_UI_Context& InContext)
    -> void
{
    // Override in derived classes to respond to context changes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnContextCleared_Implementation()
    -> void
{
    // Override in derived classes to respond to context being cleared
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ShouldInheritContextFromParent_Implementation() const
    -> bool
{
    return _InheritContextFromParent;
}

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_LayerParticipant Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    OnPrePushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = InLayerTag;
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPostPushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    // Override in derived classes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPrePopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    // Override in derived classes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPostPopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = FGameplayTag::EmptyTag;
}

// --------------------------------------------------------------------------------------------------------------------
// Context Accessors
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextEntity() const
    -> FCk_Handle
{
    return UCk_Utils_UI_Context_UE::Get_ContextEntity(this);
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextActor() const
    -> AActor*
{
    return UCk_Utils_UI_Context_UE::Get_ContextActor(this);
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextPayload() const
    -> UObject*
{
    return UCk_Utils_UI_Context_UE::Get_ContextPayload(this);
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ActivatableUserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_ActivatableUserWidget_UE::
    NativeDestruct()
    -> void
{
    if (NOT _DoNotDestroyDuringTransitions)
    {
        Super::NativeDestruct();
    }
}

auto
    UCk_ActivatableUserWidget_UE::
    NativeOnDeactivated()
    -> void
{
    if (_ClearContextWhenDeactivated)
    {
        UCk_Utils_UI_Context_UE::ClearContext(this);
    }

    Super::NativeOnDeactivated();
}

// --------------------------------------------------------------------------------------------------------------------