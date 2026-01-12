// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_LayerWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Layout/CkUI_LayerStack.h"

#include <Blueprint/WidgetTree.h>
#include <Components/Overlay.h>
#include <Components/OverlaySlot.h>

// --------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------

UCk_UI_LayerWidget_UE::UCk_UI_LayerWidget_UE(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bAutoActivate = false;
}

// --------------------------------------------------------------------------------------------------------------------
// Configuration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerWidget_UE::
    Configure(
        TSubclassOf<UCk_UI_LayerStack_UE> InStackClass,
        FGameplayTag InLayerTag,
        int32 InPriority,
        ECk_UI_InputMode InInputMode,
        EMouseCaptureMode InMouseCaptureMode)
    -> void
{
    _StackClass = InStackClass;
    _LayerTag = InLayerTag;
    _Priority = InPriority;
    _InputMode = InInputMode;
    _MouseCaptureMode = InMouseCaptureMode;
}

auto
    UCk_UI_LayerWidget_UE::
    SetTransitionSettings(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurve,
        FCk_Time InTransitionDuration)
    -> void
{
    _TransitionType = InTransitionType;
    _TransitionCurve = InTransitionCurve;
    _TransitionDuration = InTransitionDuration;
}

// --------------------------------------------------------------------------------------------------------------------
// Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerWidget_UE::
    Get_Stack() const
    -> UCk_UI_LayerStack_UE*
{
    return _Stack;
}

auto
    UCk_UI_LayerWidget_UE::
    Get_LayerTag() const
    -> FGameplayTag
{
    return _LayerTag;
}

auto
    UCk_UI_LayerWidget_UE::
    Get_Priority() const
    -> int32
{
    return _Priority;
}

auto
    UCk_UI_LayerWidget_UE::
    Get_InputMode() const
    -> ECk_UI_InputMode
{
    return _InputMode;
}

auto
    UCk_UI_LayerWidget_UE::
    Get_MouseCaptureMode() const
    -> EMouseCaptureMode
{
    return _MouseCaptureMode;
}

auto
    UCk_UI_LayerWidget_UE::
    HasWidgets() const
    -> bool
{
    return _HasWidgets;
}

auto
    UCk_UI_LayerWidget_UE::
    IsTransitioning() const
    -> bool
{
    return _IsTransitioning;
}

// --------------------------------------------------------------------------------------------------------------------
// UCommonActivatableWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerWidget_UE::
    GetDesiredInputConfig() const
    -> TOptional<FUIInputConfig>
{
    if (NOT IsActivated())
    { return {}; }

    if (NOT _HasWidgets)
    { return {}; }

    return MapInputModeToConfig(_InputMode, _MouseCaptureMode);
}

// --------------------------------------------------------------------------------------------------------------------
// UUserWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    if (ck::Is_NOT_Valid(WidgetTree))
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    if (ck::Is_NOT_Valid(_RootOverlay) && ck::IsValid(WidgetTree))
    {
        _RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
        WidgetTree->RootWidget = _RootOverlay;
    }

    DoCreateStack();

    return Super::RebuildWidget();
}

auto
    UCk_UI_LayerWidget_UE::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();
    DoBindStackEvents();
}

auto
    UCk_UI_LayerWidget_UE::
    NativeDestruct()
    -> void
{
    DoUnbindStackEvents();
    Super::NativeDestruct();
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerWidget_UE::
    DoCreateStack()
    -> void
{
    if (ck::IsValid(_Stack))
    { return; }

    if (ck::Is_NOT_Valid(WidgetTree))
    { return; }

    if (ck::Is_NOT_Valid(_RootOverlay))
    { return; }

    auto StackClass = _StackClass.Get();

    if (ck::Is_NOT_Valid(StackClass))
    {
        StackClass = UCk_UI_LayerStack_UE::StaticClass();
    }

    _Stack = WidgetTree->ConstructWidget<UCk_UI_LayerStack_UE>(StackClass, TEXT("LayerStack"));

    if (ck::Is_NOT_Valid(_Stack))
    { return; }

    _Stack->SetLayerTag(_LayerTag);
    _Stack->SetPriority(_Priority);
    _Stack->SetDefaultInputMode(_InputMode);

    if (_TransitionType.IsSet() && _TransitionCurve.IsSet() && _TransitionDuration.IsSet())
    {
        _Stack->SetTransitionSettings(
            _TransitionType.GetValue(),
            _TransitionCurve.GetValue(),
            _TransitionDuration.GetValue());
    }

    auto* OverlaySlot = Cast<UOverlaySlot>(_RootOverlay->AddChild(_Stack));

    if (ck::Is_NOT_Valid(OverlaySlot))
    { return; }

    OverlaySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
    OverlaySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
}

auto
    UCk_UI_LayerWidget_UE::
    DoBindStackEvents()
    -> void
{
    if (ck::Is_NOT_Valid(_Stack))
    { return; }

    _Stack->OnWidgetPushed.AddUObject(this, &ThisClass::HandleStackWidgetPushed);
    _Stack->OnWidgetPopped.AddUObject(this, &ThisClass::HandleStackWidgetPopped);
    _Stack->OnAllWidgetsCleared.AddUObject(this, &ThisClass::HandleStackAllWidgetsCleared);
    _Stack->OnTransitioningChanged.AddUObject(this, &ThisClass::HandleStackTransitioningChanged);
    _Stack->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::HandleStackDisplayedWidgetChanged);

    DoUpdateHasWidgets();
}

auto
    UCk_UI_LayerWidget_UE::
    DoUnbindStackEvents()
    -> void
{
    if (ck::Is_NOT_Valid(_Stack))
    { return; }

    _Stack->OnWidgetPushed.RemoveAll(this);
    _Stack->OnWidgetPopped.RemoveAll(this);
    _Stack->OnAllWidgetsCleared.RemoveAll(this);
    _Stack->OnTransitioningChanged.RemoveAll(this);
    _Stack->OnDisplayedWidgetChanged().RemoveAll(this);
}

auto
    UCk_UI_LayerWidget_UE::
    DoUpdateHasWidgets()
    -> void
{
    const auto NewHasWidgets = ck::IsValid(_Stack) && _Stack->HasActiveWidgets();

    if (_HasWidgets == NewHasWidgets)
    { return; }

    _HasWidgets = NewHasWidgets;
    OnHasWidgetsChanged.Broadcast(_HasWidgets);
}

auto
    UCk_UI_LayerWidget_UE::
    HandleStackWidgetPushed(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoUpdateHasWidgets();
    OnWidgetPushed.Broadcast(InWidget);
}

auto
    UCk_UI_LayerWidget_UE::
    HandleStackWidgetPopped(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoUpdateHasWidgets();
    OnWidgetPopped.Broadcast(InWidget);
}

auto
    UCk_UI_LayerWidget_UE::
    HandleStackAllWidgetsCleared()
    -> void
{
    DoUpdateHasWidgets();
    OnAllWidgetsCleared.Broadcast();
}

auto
    UCk_UI_LayerWidget_UE::
    HandleStackTransitioningChanged(
        UCommonActivatableWidgetContainerBase* InContainer,
        bool InIsTransitioning)
    -> void
{
    if (_IsTransitioning == InIsTransitioning)
    { return; }

    _IsTransitioning = InIsTransitioning;
    OnTransitionStateChanged.Broadcast(InIsTransitioning);
}

auto
    UCk_UI_LayerWidget_UE::
    HandleStackDisplayedWidgetChanged(
        UCommonActivatableWidget* InWidget)
    -> void
{
    DoUpdateHasWidgets();
}

auto
    UCk_UI_LayerWidget_UE::
    MapInputModeToConfig(
        ECk_UI_InputMode InMode,
        EMouseCaptureMode InCaptureMode)
    -> FUIInputConfig
{
    switch (InMode)
    {
        case ECk_UI_InputMode::GameOnly:
        {
            return FUIInputConfig(ECommonInputMode::Game, InCaptureMode);
        }
        case ECk_UI_InputMode::GameAndUI:
        {
            return FUIInputConfig(ECommonInputMode::All, InCaptureMode);
        }
        case ECk_UI_InputMode::UIOnly:
        {
            return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
        }
        default:
        {
            CK_INVALID_ENUM(InMode);
            break;
        }
    }

    return FUIInputConfig(ECommonInputMode::Game, InCaptureMode);
}

// --------------------------------------------------------------------------------------------------------------------