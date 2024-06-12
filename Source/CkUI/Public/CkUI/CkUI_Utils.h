#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/WidgetTree.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUI_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_UE);

public:
    template <typename T_Predicate>
    static auto
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UUserWidget* InUserWidget,
        T_Predicate InPred) -> void;

    template <typename T_Predicate>
    static auto
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UWidget* InWidget,
        T_Predicate InPred) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_Predicate>
auto
    UCk_Utils_UI_UE::
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UUserWidget* InUserWidget,
        T_Predicate InPred)
    -> void
{
    if (ck::Is_NOT_Valid(InUserWidget))
    { return; }

    if (auto* RootWidget = InUserWidget->GetRootWidget();
        ck::IsValid(RootWidget))
    {
        ForEachWidgetAndChildren_IncludingUserWidgets(RootWidget, InPred);
    }
}

template <typename Predicate>
auto
    UCk_Utils_UI_UE::
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UWidget* InWidget,
        Predicate InPred)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InPred(InWidget))
    { return; }

    if (const auto* UserWidget = Cast<UUserWidget>(InWidget);
        ck::IsValid(UserWidget))
    {
        if (const auto* WidgetTree = UserWidget->WidgetTree.Get();
            ck::IsValid(WidgetTree))
        {
            ForEachWidgetAndChildren_IncludingUserWidgets(WidgetTree->RootWidget, InPred);
        }
    }

    if (const auto* NamedSlotHost = Cast<INamedSlotInterface>(InWidget);
        ck::IsValid(NamedSlotHost, ck::IsValid_Policy_NullptrOnly{}))
    {
        TArray<FName> SlotNames;
        NamedSlotHost->GetSlotNames(SlotNames);

        for (const auto& SlotName : SlotNames)
        {
            auto* SlotContent = NamedSlotHost->GetContentForSlot(SlotName);

            if (ck::Is_NOT_Valid(SlotContent))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(SlotContent, InPred);
        }
    }

    if (const auto* PanelParent = Cast<UPanelWidget>(InWidget);
        ck::IsValid(PanelParent, ck::IsValid_Policy_NullptrOnly{}))
    {
        for (auto ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ++ChildIndex)
        {
            auto* ChildWidget = PanelParent->GetChildAt(ChildIndex);

            if (ck::Is_NOT_Valid(ChildWidget))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(ChildWidget, InPred);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
