// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/CkUI_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Context System - ICk_UI_ContextReceiver Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UserWidget_UE::
    OnContextInjected_Implementation(
        const FCk_UI_Context& InContext)
    -> void
{
}

auto
    UCk_UserWidget_UE::
    OnContextCleared_Implementation()
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
// Lifecycle Observer - ICk_UI_LifecycleObserver Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UserWidget_UE::
    OnPrePushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = InLayerTag;
}

auto
    UCk_UserWidget_UE::
    OnPostPushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
}

auto
    UCk_UserWidget_UE::
    OnPrePopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
}

auto
    UCk_UserWidget_UE::
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
    UCk_UserWidget_UE::
    Get_ContextEntity() const
    -> FCk_Handle
{
    return UCk_Utils_UI_UE::Get_ContextEntity(this);
}

auto
    UCk_UserWidget_UE::
    Get_ContextActor() const
    -> AActor*
{
    return UCk_Utils_UI_UE::Get_ContextActor(this);
}

auto
    UCk_UserWidget_UE::
    Get_ContextPayload() const
    -> UObject*
{
    return UCk_Utils_UI_UE::Get_ContextPayload(this);
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_UserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_UserWidget_UE::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();
}

auto
    UCk_UserWidget_UE::
    NativeDestruct()
    -> void
{
    if (NOT _DoNotDestroyDuringTransitions)
    { Super::NativeDestruct(); }
}

auto
    UCk_UserWidget_UE::
    NativeOnActivated()
    -> void
{
    Super::NativeOnActivated();
}

auto
    UCk_UserWidget_UE::
    NativeOnDeactivated()
    -> void
{
    if (_ClearContextWhenDeactivated)
    { UCk_Utils_UI_UE::ClearContext(this); }

    Super::NativeOnDeactivated();
}

// --------------------------------------------------------------------------------------------------------------------