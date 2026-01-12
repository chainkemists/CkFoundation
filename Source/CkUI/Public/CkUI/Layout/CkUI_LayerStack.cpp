// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_LayerStack.h"

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
    DoNotifyParticipantPrePush(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPostWidgetPush(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyParticipantPostPush(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPreWidgetPop(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyParticipantPrePop(InWidget);
}

auto
    UCk_UI_LayerStack_UE::
    OnPostWidgetPop(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoNotifyParticipantPostPop(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Lifecycle Notifications
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_UE::
    DoNotifyParticipantPrePush(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LayerParticipant>())
    {
        ICk_UI_LayerParticipant::Execute_OnPrePushToLayer(InWidget, _LayerTag);
    }
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyParticipantPostPush(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LayerParticipant>())
    {
        ICk_UI_LayerParticipant::Execute_OnPostPushToLayer(InWidget, _LayerTag);
    }
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyParticipantPrePop(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LayerParticipant>())
    {
        ICk_UI_LayerParticipant::Execute_OnPrePopFromLayer(InWidget, _LayerTag);
    }
}

auto
    UCk_UI_LayerStack_UE::
    DoNotifyParticipantPostPop(
        UCommonActivatableWidget* InWidget) const
    -> void
{
    if (InWidget->Implements<UCk_UI_LayerParticipant>())
    {
        ICk_UI_LayerParticipant::Execute_OnPostPopFromLayer(InWidget, _LayerTag);
    }
}

// --------------------------------------------------------------------------------------------------------------------