// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/CustomWidgets/WidgetStack/CkStack_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Types/CkUI_Types.h"

// --------------------------------------------------------------------------------------------------------------------
// Widget Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Stack_UserWidget_UE::
    PushWidgetClass(
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> UCommonActivatableWidget*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWidgetClass),
        TEXT("Cannot push invalid widget class to stack [{}]"), this)
    { return nullptr; }

    auto* Widget = AddWidget<UCommonActivatableWidget>(InWidgetClass, [this](UCommonActivatableWidget& InWidgetInstance)
    {
        OnPreWidgetPush(&InWidgetInstance);
    });

    if (ck::Is_NOT_Valid(Widget))
    { return nullptr; }

    OnPostWidgetPush(Widget);
    OnWidgetPushed.Broadcast(Widget);

    return Widget;
}

auto
    UCk_Stack_UserWidget_UE::
    PushWidgetInstance(
        UCommonActivatableWidget* InWidgetInstance)
    -> UCommonActivatableWidget*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWidgetInstance),
        TEXT("Cannot push invalid widget instance to stack [{}]"), this)
    { return nullptr; }

    CK_ENSURE_IF_NOT(NOT ContainsWidget(InWidgetInstance),
        TEXT("Widget [{}] is already in stack [{}]"), GetNameSafe(InWidgetInstance), this)
    { return nullptr; }

    OnPreWidgetPush(InWidgetInstance);
    AddWidgetInstance(*InWidgetInstance);
    OnPostWidgetPush(InWidgetInstance);

    OnWidgetPushed.Broadcast(InWidgetInstance);

    return InWidgetInstance;
}

auto
    UCk_Stack_UserWidget_UE::
    PopWidget()
    -> UCommonActivatableWidget*
{
    auto* TopWidget = GetActiveWidget();

    if (ck::Is_NOT_Valid(TopWidget))
    { return nullptr; }

    OnPreWidgetPop(TopWidget);
    RemoveWidget(*TopWidget);
    OnPostWidgetPop(TopWidget);

    OnWidgetPopped.Broadcast(TopWidget);

    return TopWidget;
}

auto
    UCk_Stack_UserWidget_UE::
    PopSpecificWidget(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    if (NOT ContainsWidget(InWidget))
    { return false; }

    OnPreWidgetPop(InWidget);
    RemoveWidget(*InWidget);
    OnPostWidgetPop(InWidget);

    OnWidgetPopped.Broadcast(InWidget);

    return true;
}

auto
    UCk_Stack_UserWidget_UE::
    ClearAllWidgets()
    -> void
{
    const auto WidgetsCopy = GetWidgetList();

    ck::algo::ForEachIsValid(WidgetsCopy, [this](UCommonActivatableWidget* InWidget)
    {
        OnPreWidgetPop(InWidget);
    });

    ClearWidgets();

    ck::algo::ForEachIsValid(WidgetsCopy, [this](UCommonActivatableWidget* InWidget)
    {
        OnPostWidgetPop(InWidget);
    });

    OnAllWidgetsCleared.Broadcast();
}

// --------------------------------------------------------------------------------------------------------------------
// Query Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Stack_UserWidget_UE::
    HasWidgets() const
    -> bool
{
    return GetNumWidgets() > 0;
}

auto
    UCk_Stack_UserWidget_UE::
    HasActiveWidgets() const
    -> bool
{
    return ck::IsValid(GetActiveWidget()) && GetActiveWidget()->IsActivated();
}

auto
    UCk_Stack_UserWidget_UE::
    ContainsWidget(
        UCommonActivatableWidget* InWidget) const
    -> bool
{
    return GetWidgetList().Contains(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Configuration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Stack_UserWidget_UE::
    SetTransitionSettings(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurve,
        FCk_Time InTransitionDuration,
        ECommonSwitcherTransitionFallbackStrategy InFallbackStrategy)
    -> void
{
    TransitionType = InTransitionType;
    TransitionCurveType = InTransitionCurve;
    SetTransitionDuration(InTransitionDuration.Get_Seconds());
    TransitionFallbackStrategy = InFallbackStrategy;
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_Stack_UserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------