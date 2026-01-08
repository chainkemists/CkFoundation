// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layer/CkUI_LayerStack.h"

#include "CommonActivatableWidget.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"

// --------------------------------------------------------------------------------------------------------------------
// Layer Configuration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_UE::
    SetLayerTag(
        FGameplayTag InTag)
    -> void
{
    _LayerTag = InTag;
}

auto
    UCk_UI_LayerStack_UE::
    SetPriority(
        int32 InPriority)
    -> void
{
    _Priority = InPriority;
}

auto
    UCk_UI_LayerStack_UE::
    SetDefaultInputMode(
        ECk_UI_InputMode InMode)
    -> void
{
    _DefaultInputMode = InMode;
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_Stack_UserWidget_UE Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_UE::
    OnPreWidgetPush(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyLifecyclePrePush(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPostWidgetPush(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyLifecyclePostPush(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPreWidgetPop(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyLifecyclePrePop(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPostWidgetPop(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyLifecyclePostPop(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Lifecycle Notifications
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_UE::
    DoNotifyLifecyclePrePush(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LifecycleObserver>())
    {
        ICk_UI_LifecycleObserver::Execute_OnPrePushToLayer(InWidget, _LayerTag);
    }
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyLifecyclePostPush(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LifecycleObserver>())
    {
        ICk_UI_LifecycleObserver::Execute_OnPostPushToLayer(InWidget, _LayerTag);
    }

    // Note: OnWidgetPushed is broadcast by the base class (UCk_Stack_UserWidget_UE)
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyLifecyclePrePop(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LifecycleObserver>())
    {
        ICk_UI_LifecycleObserver::Execute_OnPrePopFromLayer(InWidget, _LayerTag);
    }
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyLifecyclePostPop(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LifecycleObserver>())
    {
        ICk_UI_LifecycleObserver::Execute_OnPostPopFromLayer(InWidget, _LayerTag);
    }

    // Note: OnWidgetPopped is broadcast by the base class (UCk_Stack_UserWidget_UE)
}

// --------------------------------------------------------------------------------------------------------------------