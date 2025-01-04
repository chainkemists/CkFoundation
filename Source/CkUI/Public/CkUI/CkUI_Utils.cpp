#include "CkUI_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include <Blueprint/UserWidget.h>
#include <Components/NamedSlot.h>
#include <Blueprint/WidgetTree.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    FindNamedSlot(
        UUserWidget* InSourceWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive)
    -> UNamedSlot*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSourceWidget),
        TEXT("Widget source for NamedSlot [{}] is not valid"),
        InNamedSlotName)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlotName),
        TEXT("Named Slot name is not valid. Trying to get NamedSlot from widget [{}]"),
        InSourceWidget)
    { return {}; }

    auto RootWidgetTree = InSourceWidget->WidgetTree.Get();

    CK_ENSURE_IF_NOT(ck::IsValid(RootWidgetTree),
        TEXT("Widget tree of widget [{}] is not valid"),
        InSourceWidget)
    { return {}; }

    auto Widgets = TArray<UWidget*>{};
    RootWidgetTree->GetAllWidgets(Widgets);

    const auto& FoundSlotWithName = Widgets.FindByPredicate([&](UWidget* Widget)
    {
        return Widget->GetFName() == InNamedSlotName && ck::IsValid(Cast<UNamedSlot>(Widget));
    });

    if (ck::IsValid(FoundSlotWithName, ck::IsValid_Policy_NullptrOnly{}) &&
        ck::IsValid(Cast<UNamedSlot>(*FoundSlotWithName)))
    {
        return Cast<UNamedSlot>(*FoundSlotWithName);
    }

    if (InIsRecursive == ECk_UI_NamedSlot_SearchRecursive::NonRecursive)
    { return {}; }

    for (const auto& Widget : Widgets)
    {
        if (const auto& UserWidget = Cast<UUserWidget>(Widget))
        {
            if (const auto NamedSlot = FindNamedSlot(UserWidget, InNamedSlotName, InIsRecursive))
            { return NamedSlot; }
        }
    }

    return {};
}

auto
    UCk_Utils_UI_UE::
    IsNamedSlotOccupied(
        UNamedSlot* InNamedSlot)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlot),
        TEXT("Named Slot is not valid"))
    { return false; }

    return InNamedSlot->GetAllChildren().Num() > 0;
}

auto
    UCk_Utils_UI_UE::
    InsertWidgetToNamedSlot(
        UNamedSlot* InNamedSlot,
        UUserWidget* InInsertedWidget)
    -> UPanelSlot*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlot),
        TEXT("Named Slot Name is not valid, trying to insert widget [{}]"),
        InInsertedWidget)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInsertedWidget),
        TEXT("Widget to be inserted into NamedSlot [{}] is not valid"),
        InNamedSlot)
    { return {}; }

    return InNamedSlot->AddChild(InInsertedWidget);
}

auto
    UCk_Utils_UI_UE::
    FindNamedSlotAndInsertWidget(
        UUserWidget* InSourceWidget,
        UUserWidget* InInsertedWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive,
        ECk_UI_NamedSlot_EnsureSlotIsFound InEnsureSlotIsFound)
    -> UPanelSlot*
{
    if (auto NamedSlot = FindNamedSlot(InSourceWidget, InNamedSlotName, InIsRecursive);
        ck::IsValid(NamedSlot))
    {
        CK_ENSURE_IF_NOT(NOT IsNamedSlotOccupied(NamedSlot),
            TEXT("Named Slot [{}] is not available for inserting a widget, already has widget [{}] present"),
            NamedSlot,
            NamedSlot->GetAllChildren().Num() > 0 ? NamedSlot->GetAllChildren()[0] : nullptr)
        { return {}; }

        return InsertWidgetToNamedSlot(NamedSlot, InInsertedWidget);
    }

    CK_ENSURE_IF_NOT(InEnsureSlotIsFound == ECk_UI_NamedSlot_EnsureSlotIsFound::EnsureSlotIsFound,
        TEXT("Widget [{}] does not contain named slot [{}]"),
        InSourceWidget,
        InNamedSlotName)
    {}

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
