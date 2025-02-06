#include "CkStack_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Stack_UserWidget_UE::
    PushWidgetInstance(
        UCk_UserWidget_UE* InWidgetInstance)
    -> UCk_UserWidget_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWidgetInstance),
    TEXT("Trying to push an invalid widget instance to the Widget Stack [{}] owned by [{}]"), this, GetOwningPlayer())
    { return {}; }

    CK_ENSURE_IF_NOT(NOT ContainsWidgetInstance(InWidgetInstance),
    TEXT("Trying to push a widget instance [{}] that is already contained in the Widget Stack [{}] owned by [{}]"), InWidgetInstance, this, GetOwningPlayer())
    { return {}; }

    AddWidgetInstance(*InWidgetInstance);
    return InWidgetInstance;
}

auto
    UCk_Stack_UserWidget_UE::
    ContainsWidgetInstance(
        UCk_UserWidget_UE* InWidgetInstance)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWidgetInstance),
    TEXT("Widget Instance is invalid, checking if already in Widget Stack [{}] owned by [{}]"), this, GetOwningPlayer())
    { return false; }

    return WidgetList.Contains(InWidgetInstance);
}

auto
    UCk_Stack_UserWidget_UE::
    SetTransitionDetails(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurveType,
        FCk_Time InTransitionDuration,
        ECommonSwitcherTransitionFallbackStrategy InTransitionFallbackStrategy)
    -> void
{
    TransitionType = InTransitionType;
    TransitionCurveType = InTransitionCurveType;
    TransitionDuration = InTransitionDuration.Get_Seconds();
    TransitionFallbackStrategy = InTransitionFallbackStrategy;
}

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
