#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "SGameplayTagPicker.h"

#include "Styling/AppStyle.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "Ck_2dGridSystem_EdModeToolkit"

// --------------------------------------------------------------------------------------------------------------------

FCk_2dGridSystem_EdModeToolkit::FCk_2dGridSystem_EdModeToolkit()
{
}

void
    FCk_2dGridSystem_EdModeToolkit::
    Init(
        const TSharedPtr<IToolkitHost>& InToolkitHost,
        TWeakObjectPtr<UEdMode>         InOwningMode)
{
    OwningMode = InOwningMode;

    // One radio-style toggle per tool. Style as toggle buttons (ToggleButtonCheckbox) so they read as
    // a segmented selector; only one stays checked because each Get_ToolCheckState compares against
    // the EdMode's active tool.
    const auto MakeToolButton = [this](ECk_GridPaint_Tool InTool, const FText& InLabel) -> TSharedRef<SWidget>
    {
        return SNew(SCheckBox)
            .Style(FAppStyle::Get(), "ToggleButtonCheckbox")
            .Padding(FMargin(8.0f, 4.0f))
            .IsChecked(this, &FCk_2dGridSystem_EdModeToolkit::Get_ToolCheckState, InTool)
            .OnCheckStateChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_ToolCheckChanged, InTool)
            [
                SNew(STextBlock).Text(InLabel)
            ];
    };

    SAssignNew(InlineContent, SBorder)
    .Padding(8.0f)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PaintToolsLabel", "Paint Tool"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SUniformGridPanel)
            .SlotPadding(FMargin(2.0f))
            + SUniformGridPanel::Slot(0, 0)
            [
                MakeToolButton(ECk_GridPaint_Tool::Shape,   LOCTEXT("ToolShape",   "Shape"))
            ]
            + SUniformGridPanel::Slot(1, 0)
            [
                MakeToolButton(ECk_GridPaint_Tool::Tags,    LOCTEXT("ToolTags",    "Tags"))
            ]
            + SUniformGridPanel::Slot(2, 0)
            [
                MakeToolButton(ECk_GridPaint_Tool::Blocker, LOCTEXT("ToolBlocker", "Blocker"))
            ]
            + SUniformGridPanel::Slot(3, 0)
            [
                MakeToolButton(ECk_GridPaint_Tool::Select,  LOCTEXT("ToolSelect",  "Select"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            Build_TagsSection()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            Build_DetailsSection()
        ]
    ];

    FModeToolkit::Init(InToolkitHost, InOwningMode);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Build_TagsSection() -> TSharedRef<SWidget>
{
    // Scope segmented toggle (PerCellBulk vs GridDefault), styled like the tool selector.
    const auto MakeScopeButton = [this](ECk_GridPaint_TagScope InScope, const FText& InLabel) -> TSharedRef<SWidget>
    {
        return SNew(SCheckBox)
            .Style(FAppStyle::Get(), "ToggleButtonCheckbox")
            .Padding(FMargin(8.0f, 4.0f))
            .IsChecked(this, &FCk_2dGridSystem_EdModeToolkit::Get_ScopeCheckState, InScope)
            .OnCheckStateChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_ScopeCheckChanged, InScope)
            [
                SNew(STextBlock).Text(InLabel)
            ];
    };

    // Single-select tag picker writing back to the EdMode's _ActivePaintTag via On_PaintTagChanged.
    // No PropertyHandle (we drive the value imperatively) — start with an empty container.
    TagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_PaintTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_TagsSectionVisibility)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("PaintTagLabel", "Paint Tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                TagPicker.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("TagScopeLabel", "Scope"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(2.0f))
                + SUniformGridPanel::Slot(0, 0)
                [
                    MakeScopeButton(ECk_GridPaint_TagScope::PerCellBulk, LOCTEXT("ScopePerCell", "Per-Cell Bulk"))
                ]
                + SUniformGridPanel::Slot(1, 0)
                [
                    MakeScopeButton(ECk_GridPaint_TagScope::GridDefault, LOCTEXT("ScopeGridDefault", "Grid Default"))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(SUniformGridPanel)
                .SlotPadding(FMargin(2.0f))
                + SUniformGridPanel::Slot(0, 0)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .IsEnabled(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridDefaultButtonsEnabled)
                    .OnClicked(this, &FCk_2dGridSystem_EdModeToolkit::On_ApplyGridDefaultTag)
                    .ToolTipText(LOCTEXT("ApplyGridDefaultTip", "Add the active tag to the grid's DefaultCellTags"))
                    [
                        SNew(STextBlock).Text(LOCTEXT("ApplyGridDefault", "Apply Grid-Default Tag"))
                    ]
                ]
                + SUniformGridPanel::Slot(1, 0)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .IsEnabled(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridDefaultButtonsEnabled)
                    .OnClicked(this, &FCk_2dGridSystem_EdModeToolkit::On_RemoveGridDefaultTag)
                    .ToolTipText(LOCTEXT("RemoveGridDefaultTip", "Remove the active tag from the grid's DefaultCellTags"))
                    [
                        SNew(STextBlock).Text(LOCTEXT("RemoveGridDefault", "Remove"))
                    ]
                ]
            ]
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Build_DetailsSection() -> TSharedRef<SWidget>
{
    // Read-only cell inspector. Every value text block is bound to a getter that reads the EdMode's
    // selected-cell info off the Spec each frame, so it tracks live edits to the Spec.
    const auto MakeRow = [](const FText& InLabel, TAttribute<FText> InValue) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SNew(STextBlock).Text(InLabel)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(InValue)
                .AutoWrapText(true)
            ];
    };

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsSectionVisibility)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("DetailsLabel", "Cell Details"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 2.0f)
            [
                MakeRow(LOCTEXT("DetailsCoordinate", "Coordinate:"),
                    TAttribute<FText>(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsCoordinateText))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 2.0f)
            [
                MakeRow(LOCTEXT("DetailsState", "State:"),
                    TAttribute<FText>(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsStateText))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 2.0f)
            [
                MakeRow(LOCTEXT("DetailsCellTags", "Cell tags:"),
                    TAttribute<FText>(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsCellTagsText))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeRow(LOCTEXT("DetailsGridDefaultTags", "Grid-default tags:"),
                    TAttribute<FText>(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsGridDefaultTagsText))
            ]
        ];
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsSectionVisibility() const -> EVisibility
{
    return Get_ActiveTool() == ECk_GridPaint_Tool::Select
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsCoordinateText() const -> FText
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return LOCTEXT("DetailsNoCell", "Select a cell"); }

    const auto Info = Mode->Resolve_SelectedCellInfo();
    if (! Info.bHasSelection)
    { return LOCTEXT("DetailsNoCell", "Select a cell"); }

    return FText::Format(LOCTEXT("DetailsCoordValue", "({0}, {1})"),
        FText::AsNumber(Info.Coordinate.X), FText::AsNumber(Info.Coordinate.Y));
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsStateText() const -> FText
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FText::GetEmpty(); }

    const auto Info = Mode->Resolve_SelectedCellInfo();
    if (! Info.bHasSelection)
    { return FText::GetEmpty(); }

    switch (Info.State)
    {
        case UCk_2dGridSystem_EdMode::ECellState::Disabled:
        {
            return LOCTEXT("DetailsStateDisabled", "Disabled");
        }
        case UCk_2dGridSystem_EdMode::ECellState::Blocked:
        {
            return FText::Format(LOCTEXT("DetailsStateBlocked", "Blocked (blocker #{0})"),
                FText::AsNumber(Info.BlockerIndex));
        }
        default:
        {
            return LOCTEXT("DetailsStateEnabled", "Enabled");
        }
    }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsCellTagsText() const -> FText
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FText::GetEmpty(); }

    const auto Info = Mode->Resolve_SelectedCellInfo();
    if (! Info.bHasSelection)
    { return FText::GetEmpty(); }

    if (Info.CellTags.IsEmpty())
    { return LOCTEXT("DetailsTagsNone", "(none)"); }

    return FText::FromString(Info.CellTags.ToStringSimple());
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsGridDefaultTagsText() const -> FText
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FText::GetEmpty(); }

    const auto Info = Mode->Resolve_SelectedCellInfo();
    if (! Info.bHasSelection)
    { return FText::GetEmpty(); }

    if (Info.GridDefaultTags.IsEmpty())
    { return LOCTEXT("DetailsTagsNone", "(none)"); }

    return FText::FromString(Info.GridDefaultTags.ToStringSimple());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Set_ActiveTool(
        ECk_GridPaint_Tool InTool) -> void
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Set_ActiveTool(InTool); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_ActiveTool() const -> ECk_GridPaint_Tool
{
    if (const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { return Mode->Get_ActiveTool(); }

    return ECk_GridPaint_Tool::Shape;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_ToolCheckState(
        ECk_GridPaint_Tool InTool) const -> ECheckBoxState
{
    return Get_ActiveTool() == InTool
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_ToolCheckChanged(
        ECheckBoxState     InNewState,
        ECk_GridPaint_Tool InTool) -> void
{
    // Radio semantics: ignore the un-check that fires when the user clicks the already-active tool;
    // only act on a positive selection.
    if (InNewState != ECheckBoxState::Checked)
    { return; }

    Set_ActiveTool(InTool);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_TagsSectionVisibility() const -> EVisibility
{
    return Get_ActiveTool() == ECk_GridPaint_Tool::Tags
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_PaintTagChanged(
        const TArray<FGameplayTagContainer>& InContainers) -> void
{
    // Single-select picker: take the first tag of the first container (mirrors SGameplayTagCombo).
    const auto NewTag = InContainers.IsEmpty() ? FGameplayTag() : InContainers[0].First();

    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Set_ActivePaintTag(NewTag); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_ScopeCheckState(
        ECk_GridPaint_TagScope InScope) const -> ECheckBoxState
{
    if (const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    {
        return Mode->Get_TagScope() == InScope
            ? ECheckBoxState::Checked
            : ECheckBoxState::Unchecked;
    }

    return ECheckBoxState::Unchecked;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_ScopeCheckChanged(
        ECheckBoxState         InNewState,
        ECk_GridPaint_TagScope InScope) -> void
{
    if (InNewState != ECheckBoxState::Checked)
    { return; }

    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Set_TagScope(InScope); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_GridDefaultButtonsEnabled() const -> bool
{
    if (const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { return Mode->Get_TagScope() == ECk_GridPaint_TagScope::GridDefault; }

    return false;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_ApplyGridDefaultTag() -> FReply
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Apply_GridDefaultTag(); }

    return FReply::Handled();
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_RemoveGridDefaultTag() -> FReply
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Remove_GridDefaultTag(); }

    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------

void
    FCk_2dGridSystem_EdModeToolkit::
    GetToolPaletteNames(
        TArray<FName>& OutPaletteNames) const
{
    OutPaletteNames.Add(NAME_Default);
}

FName
    FCk_2dGridSystem_EdModeToolkit::
    GetToolkitFName() const
{
    return FName("Ck_2dGridSystem_EdMode");
}

FText
    FCk_2dGridSystem_EdModeToolkit::
    GetBaseToolkitName() const
{
    return LOCTEXT("DisplayName", "Grid Paint");
}

TSharedPtr<SWidget>
    FCk_2dGridSystem_EdModeToolkit::
    GetInlineContent() const
{
    return InlineContent;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
