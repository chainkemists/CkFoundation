// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_Layout_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Layout/CkUI_Layout_Subsystem.h"
#include "CkUI/Layout/CkUI_PrimaryGameLayout.h"
#include "CkUI/Layout/CkUI_LayerStack.h"

#include <CommonActivatableWidget.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Layout_UE::
    Get_LayoutSubsystem(
        const APlayerController* InPlayerController)
    -> UCk_UI_Layout_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return nullptr; }

    return Get_LayoutSubsystem(InPlayerController->GetLocalPlayer());
}

auto
    UCk_Utils_UI_Layout_UE::
    Get_LayoutSubsystem(
        const ULocalPlayer* InLocalPlayer)
    -> UCk_UI_Layout_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return nullptr; }

    return InLocalPlayer->GetSubsystem<UCk_UI_Layout_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------
// Layout Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Layout_UE::
    Get_Layout(
        const APlayerController* InPlayerController)
    -> UCk_UI_PrimaryGameLayout_UE*
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->Get_Layout();
}

auto
    UCk_Utils_UI_Layout_UE::
    Get_Layer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> UCk_UI_LayerStack_UE*
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->Get_Layer(InLayerTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Widget Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Layout_UE::
    PushWidgetToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PushWidgetToLayer(InLayerTag, InWidgetClass);
}

auto
    UCk_Utils_UI_Layout_UE::
    PushWidgetToLayer_Soft(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady)
    -> void
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    {
        InOnWidgetReady.ExecuteIfBound(nullptr);
        return;
    }

    Subsystem->PushWidgetToLayer_Soft(InLayerTag, InWidgetClass, InOnWidgetReady);
}

auto
    UCk_Utils_UI_Layout_UE::
    PushWidgetToLayer_Instance(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PushWidgetToLayer_Instance(InLayerTag, InWidget);
}

auto
    UCk_Utils_UI_Layout_UE::
    PopWidgetFromLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PopWidgetFromLayer(InLayerTag);
}

auto
    UCk_Utils_UI_Layout_UE::
    ClearLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> void
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ClearLayer(InLayerTag);
}

auto
    UCk_Utils_UI_Layout_UE::
    RemoveWidget(
        const APlayerController* InPlayerController,
        UCommonActivatableWidget* InWidget)
    -> bool
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->RemoveWidget(InWidget);
}

auto
    UCk_Utils_UI_Layout_UE::
    RemoveWidgetSelf(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    return RemoveWidget(InWidget->GetOwningPlayer(), InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Input Mode
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Layout_UE::
    Get_EffectiveInputMode(
        const APlayerController* InPlayerController)
    -> ECk_UI_InputMode
{
    auto* Subsystem = Get_LayoutSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return ECk_UI_InputMode::GameOnly; }

    return Subsystem->Get_EffectiveInputMode();
}

// --------------------------------------------------------------------------------------------------------------------