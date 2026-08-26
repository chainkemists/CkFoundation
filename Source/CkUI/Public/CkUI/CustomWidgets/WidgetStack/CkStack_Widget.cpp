// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/CustomWidgets/WidgetStack/CkStack_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUICore/Types/CkUI_Types.h"
#include "CkUICore/UserWidget/CkActivatableWidget.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_WidgetStack_UE::
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

    // The container owns the Slate tree while the widget is displayed — the keep-alive ref is
    // only needed across the pooled (popped) window, and is re-taken on pop.
    DoForgetKeptSlate(Widget);

    OnPostWidgetPush(Widget);
    OnWidgetPushed.Broadcast(Widget);

    return Widget;
}

auto
    UCk_WidgetStack_UE::
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
    DoForgetKeptSlate(InWidgetInstance);
    OnPostWidgetPush(InWidgetInstance);

    OnWidgetPushed.Broadcast(InWidgetInstance);

    return InWidgetInstance;
}

auto
    UCk_WidgetStack_UE::
    PopWidget()
    -> UCommonActivatableWidget*
{
    auto* TopWidget = GetActiveWidget();

    if (ck::Is_NOT_Valid(TopWidget))
    { return nullptr; }

    OnPreWidgetPop(TopWidget);
    DoTryKeepSlateAlive(TopWidget);
    RemoveWidget(*TopWidget);
    OnPostWidgetPop(TopWidget);

    OnWidgetPopped.Broadcast(TopWidget);

    return TopWidget;
}

auto
    UCk_WidgetStack_UE::
    PopSpecificWidget(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    if (NOT ContainsWidget(InWidget))
    { return false; }

    OnPreWidgetPop(InWidget);
    DoTryKeepSlateAlive(InWidget);
    RemoveWidget(*InWidget);
    OnPostWidgetPop(InWidget);

    OnWidgetPopped.Broadcast(InWidget);

    return true;
}

auto
    UCk_WidgetStack_UE::
    ClearAllWidgets()
    -> void
{
    const auto WidgetsCopy = GetWidgetList();

    ck::algo::ForEachIsValid(WidgetsCopy, [this](UCommonActivatableWidget* InWidget)
    {
        OnPreWidgetPop(InWidget);
        DoTryKeepSlateAlive(InWidget);
    });

    ClearWidgets();

    ck::algo::ForEachIsValid(WidgetsCopy, [this](UCommonActivatableWidget* InWidget)
    {
        OnPostWidgetPop(InWidget);
    });

    OnAllWidgetsCleared.Broadcast();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_WidgetStack_UE::
    PreWarmWidgetClass(
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> void
{
    if (ck::Is_NOT_Valid(InWidgetClass))
    { return; }

    const auto* WidgetCDO = Cast<UCk_ActivatableWidget_UE>(InWidgetClass->GetDefaultObject());
    if (WidgetCDO == nullptr || NOT WidgetCDO->Get_KeepSlateAliveWhenPooled())
    { return; }

    auto* Widget = GeneratedWidgetsPool.GetOrCreateInstance(InWidgetClass);
    if (ck::Is_NOT_Valid(Widget))
    { return; }

    // TakeWidget builds the authored tree NOW, off the interaction path (Construct fires here,
    // once — the opt-in flag's documented contract). Our ref outlives the pool release below, so
    // the first real push finds MyGCWidget alive and skips RebuildWidget entirely.
    _KeptAliveSlate.Add(FObjectKey{Widget}, Widget->TakeWidget());
    GeneratedWidgetsPool.Release(Widget, /*bReleaseSlate=*/true);
}

auto
    UCk_WidgetStack_UE::
    DoTryKeepSlateAlive(
        UCommonActivatableWidget* InWidget)
    -> void
{
    const auto* CkWidget = Cast<UCk_ActivatableWidget_UE>(InWidget);
    if (CkWidget == nullptr || NOT CkWidget->Get_KeepSlateAliveWhenPooled())
    { return; }

    const auto CachedWidget = InWidget->GetCachedWidget();
    if (NOT CachedWidget.IsValid())
    { return; }

    _KeptAliveSlate.Add(FObjectKey{InWidget}, CachedWidget);
}

auto
    UCk_WidgetStack_UE::
    DoForgetKeptSlate(
        UCommonActivatableWidget* InWidget)
    -> void
{
    if (InWidget == nullptr)
    { return; }

    _KeptAliveSlate.Remove(FObjectKey{InWidget});
}

auto
    UCk_WidgetStack_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);

    // The stack's own Slate is going away (world/layout teardown) — kept trees must go with it,
    // or they would outlive the viewport they were built for.
    _KeptAliveSlate.Empty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_WidgetStack_UE::
    HasWidgets() const
    -> bool
{
    return GetNumWidgets() > 0;
}

auto
    UCk_WidgetStack_UE::
    HasActiveWidgets() const
    -> bool
{
    return ck::IsValid(GetActiveWidget()) && GetActiveWidget()->IsActivated();
}

auto
    UCk_WidgetStack_UE::
    ContainsWidget(
        UCommonActivatableWidget* InWidget) const
    -> bool
{
    return GetWidgetList().Contains(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_WidgetStack_UE::
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

#if WITH_EDITOR
auto
    UCk_WidgetStack_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------