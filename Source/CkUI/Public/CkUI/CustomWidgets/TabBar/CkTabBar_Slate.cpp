#include "CkTabBar_Slate.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Components/ActorComponent.h>
#include <GameFramework/Actor.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_TabBar::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _Style = ck::IsValid(InArgs._Style, ck::IsValid_Policy_NullptrOnly{})
        ? InArgs._Style
        : &FCk_TabBar_Style::Get_Default();

    _TabConfigs = InArgs._TabConfigs;
    _Orientation = InArgs._Orientation;
    _OnTabSelected = InArgs._OnTabSelected;

    _ActiveTabIndex = _TabConfigs.IsEmpty()
        ? 0
        : FMath::Clamp(InArgs._InitialTabIndex, 0, _TabConfigs.Num() - 1);

    DoRebuildTabBar();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_TabBar::
    Set_Style(
        const FCk_TabBar_Style* InStyle)
    -> void
{
    _Style = ck::IsValid(InStyle, ck::IsValid_Policy_NullptrOnly{})
        ? InStyle
        : &FCk_TabBar_Style::Get_Default();

    DoRebuildTabBar();
}

auto
    SCk_TabBar::
    Set_Orientation(
        EOrientation InOrientation)
    -> void
{
    if (_Orientation == InOrientation)
    { return; }

    _Orientation = InOrientation;
    DoRebuildTabBar();
}

auto
    SCk_TabBar::
    Set_TabConfigs(
        const TArray<FCk_TabBar_TabConfig>& InTabConfigs)
    -> void
{
    _TabConfigs = InTabConfigs;

    _ActiveTabIndex = _TabConfigs.IsEmpty()
        ? 0
        : FMath::Clamp(_ActiveTabIndex, 0, _TabConfigs.Num() - 1);

    DoRebuildTabBar();
}

auto
    SCk_TabBar::
    Set_TabConfig(
        int32 InIndex,
        const FCk_TabBar_TabConfig& InTabConfig)
    -> void
{
    if (NOT _TabConfigs.IsValidIndex(InIndex))
    { return; }

    _TabConfigs[InIndex] = InTabConfig;
    DoRebuildTabBar();
}

auto
    SCk_TabBar::
    Set_ActiveTab(
        int32 InIndex)
    -> void
{
    if (NOT _TabConfigs.IsValidIndex(InIndex))
    { return; }

    if (_ActiveTabIndex == InIndex)
    { return; }

    const auto PreviousIndex = _ActiveTabIndex;
    _ActiveTabIndex = InIndex;

    DoApplyTabStyle(PreviousIndex);
    DoApplyTabStyle(InIndex);

    _OnTabSelected.ExecuteIfBound(InIndex);
}

auto
    SCk_TabBar::
    Get_ActiveTabIndex() const
    -> int32
{
    return _ActiveTabIndex;
}

auto
    SCk_TabBar::
    Get_TabCount() const
    -> int32
{
    return _TabConfigs.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_TabBar::
    DoRebuildTabBar()
    -> void
{
    _TabButtons.Reset();
    _TabLabels.Reset();

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(&_Style->Get_TabBarBackground())
        .Padding(0.0f)
        [
            SNew(SScrollBox)
            .Orientation(_Orientation)
            .ScrollBarVisibility(EVisibility::Collapsed)
            + SScrollBox::Slot()
            [
                DoBuildTabPanel()
            ]
        ]
    ];
}

auto
    SCk_TabBar::
    DoBuildTabPanel()
    -> TSharedRef<SWidget>
{
    if (_Orientation == EOrientation::Orient_Horizontal)
    {
        const auto TabPanel = SNew(SHorizontalBox);

        for (auto Index = 0; Index < _TabConfigs.Num(); ++Index)
        {
            TabPanel->AddSlot()
                .AutoWidth()
                .Padding(Index > 0 ? FMargin{_Style->Get_TabSpacing(), 0.0f, 0.0f, 0.0f} : FMargin{0.0f})
                [
                    DoBuildTabButton(Index)
                ];
        }

        return TabPanel;
    }

    const auto TabPanel = SNew(SVerticalBox);

    for (auto Index = 0; Index < _TabConfigs.Num(); ++Index)
    {
        TabPanel->AddSlot()
            .AutoHeight()
            .Padding(Index > 0 ? FMargin{0.0f, _Style->Get_TabSpacing(), 0.0f, 0.0f} : FMargin{0.0f})
            [
                DoBuildTabButton(Index)
            ];
    }

    return TabPanel;
}

auto
    SCk_TabBar::
    DoBuildTabButton(
        int32 InTabIndex)
    -> TSharedRef<SWidget>
{
    const auto& Config = _TabConfigs[InTabIndex];
    const auto IsActiveTab = InTabIndex == _ActiveTabIndex;

    const auto ButtonContent = SNew(SHorizontalBox);

    const auto HasIcon = ck::IsValid(Config.Get_TabIcon().GetResourceObject(), ck::IsValid_Policy_NullptrOnly{}) ||
        Config.Get_TabIcon().GetResourceName() != NAME_None;

    if (HasIcon)
    {
        constexpr auto IconToLabelSpacing = 4.0f;

        ButtonContent->AddSlot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin{0.0f, 0.0f, Config.Get_TabName().IsEmpty() ? 0.0f : IconToLabelSpacing, 0.0f})
            [
                SNew(SImage)
                .Image(&Config.Get_TabIcon())
                .DesiredSizeOverride(_Style->Get_IconSize())
            ];
    }

    auto TabLabel = TSharedPtr<STextBlock>{};
    if (NOT Config.Get_TabName().IsEmpty())
    {
        ButtonContent->AddSlot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SAssignNew(TabLabel, STextBlock)
                .TextStyle(IsActiveTab ? &_Style->Get_ActiveTabTextStyle() : &_Style->Get_TabTextStyle())
                .Text(Config.Get_TabName())
            ];
    }

    const auto TabButton = SNew(SButton)
        .ButtonStyle(IsActiveTab ? &_Style->Get_ActiveTabButtonStyle() : &_Style->Get_TabButtonStyle())
        .ContentPadding(_Style->Get_TabPadding())
        .OnClicked(this, &SCk_TabBar::HandleTabClicked, InTabIndex)
        [
            ButtonContent
        ];

    _TabButtons.Add(TabButton);
    _TabLabels.Add(TabLabel);

    return TabButton;
}

auto
    SCk_TabBar::
    DoApplyTabStyle(
        int32 InTabIndex)
    -> void
{
    if (NOT _TabButtons.IsValidIndex(InTabIndex))
    { return; }

    const auto IsActiveTab = InTabIndex == _ActiveTabIndex;

    if (const auto& TabButton = _TabButtons[InTabIndex];
        ck::IsValid(TabButton))
    {
        TabButton->SetButtonStyle(IsActiveTab ? &_Style->Get_ActiveTabButtonStyle() : &_Style->Get_TabButtonStyle());
    }

    if (const auto& TabLabel = _TabLabels[InTabIndex];
        ck::IsValid(TabLabel))
    {
        TabLabel->SetTextStyle(IsActiveTab ? &_Style->Get_ActiveTabTextStyle() : &_Style->Get_TabTextStyle());
    }
}

auto
    SCk_TabBar::
    HandleTabClicked(
        int32 InTabIndex)
    -> FReply
{
    Set_ActiveTab(InTabIndex);
    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------
