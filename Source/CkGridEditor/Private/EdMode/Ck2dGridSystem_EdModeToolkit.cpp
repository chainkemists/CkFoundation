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
            Build_BlockerSection()
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
    Build_BlockerSection() -> TSharedRef<SWidget>
{
    // New-blocker tag picker: writes the EdMode's _ActiveBlockerTag, stamped onto the next drag-rect
    // blocker. Single-select, mirroring the Tags-tool picker. No PropertyHandle (driven imperatively).
    NewBlockerTagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_NewBlockerTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    // Selected-blocker tag picker: writes the chosen tag to the currently selected blocker's Name via
    // Set_SelectedBlockerName. Its displayed value is re-seeded imperatively whenever the selected
    // blocker index changes (see Get_SelectedBlockerText, called live each frame).
    SelectedBlockerTagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_SelectedBlockerTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_BlockerSectionVisibility)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("NewBlockerTagLabel", "New blocker tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                NewBlockerTagPicker.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(this, &FCk_2dGridSystem_EdModeToolkit::Get_SelectedBlockerText)
                .AutoWrapText(true)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SBox)
                .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_SelectedBlockerEditorVisibility)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 4.0f)
                    [
                        SNew(STextBlock).Text(LOCTEXT("SelectedBlockerTagLabel", "Selected blocker tag"))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SelectedBlockerTagPicker.ToSharedRef()
                    ]
                ]
            ]
        ];
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_BlockerSectionVisibility() const -> EVisibility
{
    return Get_ActiveTool() == ECk_GridPaint_Tool::Blocker
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_NewBlockerTagChanged(
        const TArray<FGameplayTagContainer>& InContainers) -> void
{
    // Single-select picker: take the first tag of the first container (mirrors the Tags-tool picker).
    const auto NewTag = InContainers.IsEmpty() ? FGameplayTag() : InContainers[0].First();

    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Set_ActiveBlockerTag(NewTag); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_SelectedBlockerTagChanged(
        const TArray<FGameplayTagContainer>& InContainers) -> void
{
    const auto NewTag = InContainers.IsEmpty() ? FGameplayTag() : InContainers[0].First();

    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return; }

    // Ignore writes when no blocker is selected (the picker still emits an OnTagChanged after a re-seed
    // to the empty container). Set_SelectedBlockerName is itself a no-op for INDEX_NONE, but guarding
    // here also keeps SeededSelectedBlockerIndex honest below.
    if (Mode->Get_SelectedBlockerIndex() == INDEX_NONE)
    { return; }

    Mode->Set_SelectedBlockerName(NewTag);

    // The picker now reflects this tag for the current selection; record it so Get_SelectedBlockerText
    // does not immediately re-seed (and clobber) the value the user just chose.
    SeededSelectedBlockerIndex = Mode->Get_SelectedBlockerIndex();
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_SelectedBlockerEditorVisibility() const -> EVisibility
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return EVisibility::Collapsed; }

    return Mode->Get_SelectedBlockerIndex() != INDEX_NONE
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_SelectedBlockerText() const -> FText
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return LOCTEXT("SelectedBlockerNone", "Selected blocker: none selected"); }

    const auto Index = Mode->Get_SelectedBlockerIndex();

    // Re-seed the selected-blocker picker's displayed value when the selection changes (the picker has
    // no live-bound value attribute, so we drive it imperatively here — this getter is called each
    // frame by the live-bound text block). const_cast: this getter is logically const but owns the
    // toolkit's view-state bookkeeping.
    if (Index != SeededSelectedBlockerIndex)
    {
        auto* MutableThis = const_cast<FCk_2dGridSystem_EdModeToolkit*>(this);
        MutableThis->SeededSelectedBlockerIndex = Index;

        if (SelectedBlockerTagPicker.IsValid())
        {
            auto Container = FGameplayTagContainer{};
            const auto SelectedTag = Mode->Get_SelectedBlockerName();
            if (SelectedTag.IsValid())
            { Container.AddTag(SelectedTag); }

            SelectedBlockerTagPicker->SetTagContainers(TArray<FGameplayTagContainer>{ Container });
        }
    }

    if (Index == INDEX_NONE)
    { return LOCTEXT("SelectedBlockerNone", "Selected blocker: none selected"); }

    const auto SelectedTag = Mode->Get_SelectedBlockerName();
    if (! SelectedTag.IsValid())
    {
        return FText::Format(LOCTEXT("SelectedBlockerUnnamed", "Selected blocker: #{0} (unnamed)"),
            FText::AsNumber(Index));
    }

    return FText::Format(LOCTEXT("SelectedBlockerNamed", "Selected blocker: #{0} (tag: {1})"),
        FText::AsNumber(Index), FText::FromName(SelectedTag.GetTagName()));
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
            const auto TagText = Info.BlockerName.IsValid()
                ? FText::FromName(Info.BlockerName.GetTagName())
                : LOCTEXT("DetailsBlockerUnnamed", "(unnamed)");

            return FText::Format(LOCTEXT("DetailsStateBlocked", "Blocked (blocker #{0}, tag: {1})"),
                FText::AsNumber(Info.BlockerIndex), TagText);
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
