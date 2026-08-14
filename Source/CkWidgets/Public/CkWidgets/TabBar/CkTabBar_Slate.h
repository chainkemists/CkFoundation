#pragma once

#include "CkWidgets/TabBar/CkTabBar_Types.h"

#include <CoreMinimal.h>
#include <Input/Reply.h>
#include <Types/SlateEnums.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SCompoundWidget.h>

// --------------------------------------------------------------------------------------------------------------------

class SButton;
class STextBlock;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_OneParam(FCk_Delegate_TabBar_TabSelected, int32);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Row (or column) of selectable tab buttons, each with an optional icon and label.
 * Selecting a tab restyles the affected buttons in place and fires OnTabSelected —
 * the bar itself does not switch any content; bind OnTabSelected to do that.
 */
class CKWIDGETS_API SCk_TabBar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCk_TabBar)
        : _Style(nullptr)
        , _Orientation(EOrientation::Orient_Horizontal)
        , _InitialTabIndex(0)
    {}
        // Styling for the bar and its buttons. Falls back to FCk_TabBar_Style::Get_Default when unset.
        SLATE_ARGUMENT(const FCk_TabBar_Style*, Style)

        SLATE_ARGUMENT(TArray<FCk_TabBar_TabConfig>, TabConfigs)

        SLATE_ARGUMENT(EOrientation, Orientation)

        SLATE_ARGUMENT(int32, InitialTabIndex)

        // Invoked with the newly selected tab index whenever the selection changes.
        SLATE_EVENT(FCk_Delegate_TabBar_TabSelected, OnTabSelected)
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs) -> void;

public:
    auto Set_Style(const FCk_TabBar_Style* InStyle) -> void;
    auto Set_Orientation(EOrientation InOrientation) -> void;
    auto Set_TabConfigs(const TArray<FCk_TabBar_TabConfig>& InTabConfigs) -> void;
    auto Set_TabConfig(int32 InIndex, const FCk_TabBar_TabConfig& InTabConfig) -> void;
    auto Set_ActiveTab(int32 InIndex) -> void;

    auto Get_ActiveTabIndex() const -> int32;
    auto Get_TabCount() const -> int32;

private:
    auto DoRebuildTabBar() -> void;
    auto DoBuildTabPanel() -> TSharedRef<SWidget>;
    auto DoBuildTabButton(int32 InTabIndex) -> TSharedRef<SWidget>;
    auto DoApplyTabStyle(int32 InTabIndex) -> void;
    auto HandleTabClicked(int32 InTabIndex) -> FReply;

private:
    const FCk_TabBar_Style* _Style = nullptr;
    TArray<FCk_TabBar_TabConfig> _TabConfigs;
    EOrientation _Orientation = EOrientation::Orient_Horizontal;
    int32 _ActiveTabIndex = 0;

    TArray<TSharedPtr<SButton>> _TabButtons;
    TArray<TSharedPtr<STextBlock>> _TabLabels;

    FCk_Delegate_TabBar_TabSelected _OnTabSelected;
};

// --------------------------------------------------------------------------------------------------------------------
