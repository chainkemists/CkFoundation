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
        .Filter(TEXT("Grid"))
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
        .Filter(TEXT("Grid"))
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_NewBlockerTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    // Selected-blocker tag picker: writes the chosen tag to the currently selected blocker's Name via
    // Set_SelectedBlockerName. Its displayed value is re-seeded imperatively whenever the selected
    // blocker index changes (see Get_SelectedBlockerText, called live each frame).
    SelectedBlockerTagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .Filter(TEXT("Grid"))
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
    // Live-bound label/value row helper. Value text blocks read the EdMode's selected-cell info off the
    // Spec each frame, so they track live edits to the Spec.
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

    // Add-tag picker for the single-cell editor: each chosen tag is ADDED to the selected cell.
    AddCellTagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .Filter(TEXT("Grid"))
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_AddCellTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    // Vertical list of one removable row per tag on the selected cell. Rebuilt imperatively whenever the
    // cell or its tag set changes (see Rebuild_PerCellTagList / Compute_PerCellTagListSignature).
    SAssignNew(PerCellTagListContainer, SVerticalBox);

    // Blocker editor tag picker: writes the selected blocker's Name via the shared Set_SelectedBlockerName.
    DetailsBlockerTagPicker = SNew(SGameplayTagPicker)
        .MultiSelect(false)
        .Filter(TEXT("Grid"))
        .GameplayTagPickerMode(EGameplayTagPickerMode::SelectionMode)
        .MaxHeight(250.0f)
        .OnTagChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_DetailsBlockerTagChanged)
        .TagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} });

    // Seed the per-cell tag list once up-front (empty until a cell is selected).
    Rebuild_PerCellTagList();

    // Single-CELL editor sub-block.
    const auto CellEditor = SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsCellEditorVisibility)
        [
            SNew(SVerticalBox)
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
            .Padding(0.0f, 2.0f, 0.0f, 2.0f)
            [
                SNew(SCheckBox)
                .IsChecked(this, &FCk_2dGridSystem_EdModeToolkit::Get_SelectedCellDisabledState)
                .OnCheckStateChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_SelectedCellDisabledChanged)
                [
                    SNew(STextBlock).Text(LOCTEXT("DetailsDisabledToggle", "Disabled"))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 2.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("DetailsCellTagsLabel", "Cell tags:"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                PerCellTagListContainer.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 2.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("DetailsAddCellTagLabel", "Add cell tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                AddCellTagPicker.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                MakeRow(LOCTEXT("DetailsGridDefaultTags", "Grid-default tags:"),
                    TAttribute<FText>(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsGridDefaultTagsText))
            ]
        ];

    // BLOCKER editor sub-block (shown when the pick landed on a blocker).
    const auto BlockerEditor = SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsBlockerEditorVisibility)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(STextBlock)
                .Text(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsBlockerText)
                .AutoWrapText(true)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 2.0f)
            [
                SNew(STextBlock).Text(LOCTEXT("DetailsBlockerTagLabel", "Blocker tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                DetailsBlockerTagPicker.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .OnClicked(this, &FCk_2dGridSystem_EdModeToolkit::On_DeleteSelectedBlocker)
                .ToolTipText(LOCTEXT("DetailsDeleteBlockerTip", "Remove this blocker from the grid"))
                [
                    SNew(STextBlock).Text(LOCTEXT("DetailsDeleteBlocker", "Delete Blocker"))
                ]
            ]
        ];

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
            [
                CellEditor
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                BlockerEditor
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
    Get_DetailsCellEditorVisibility() const -> EVisibility
{
    if (Get_ActiveTool() != ECk_GridPaint_Tool::Select)
    { return EVisibility::Collapsed; }

    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return EVisibility::Collapsed; }

    // Single-cell editor is shown for any non-blocker Select pick (including no pick yet — then the
    // coordinate row reads "Select a cell").
    return Mode->Get_HasBlockerSelection()
        ? EVisibility::Collapsed
        : EVisibility::Visible;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsBlockerEditorVisibility() const -> EVisibility
{
    if (Get_ActiveTool() != ECk_GridPaint_Tool::Select)
    { return EVisibility::Collapsed; }

    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return EVisibility::Collapsed; }

    return Mode->Get_HasBlockerSelection()
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsCoordinateText() const -> FText
{
    // This getter is live-bound (called each repaint while the cell editor is visible); piggyback the
    // per-cell tag-list rebuild here so the removable rows track live edits without a manual refresh.
    // const_cast: logically const, but it owns the toolkit's view-state bookkeeping (mirrors the Blocker
    // section's Get_SelectedBlockerText re-seed).
    {
        const auto Signature = Compute_PerCellTagListSignature();
        if (Signature != SeededPerCellTagSignature)
        {
            auto* MutableThis = const_cast<FCk_2dGridSystem_EdModeToolkit*>(this);
            MutableThis->SeededPerCellTagSignature = Signature;
            MutableThis->Rebuild_PerCellTagList();
        }
    }

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
    Get_SelectedCellDisabledState() const -> ECheckBoxState
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return ECheckBoxState::Unchecked; }

    return Mode->Get_SelectedCellDisabled()
        ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_SelectedCellDisabledChanged(
        ECheckBoxState InNewState) -> void
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Set_SelectedCellDisabled(InNewState == ECheckBoxState::Checked); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Compute_PerCellTagListSignature() const -> FString
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FString{}; }

    const auto Info = Mode->Resolve_SelectedCellInfo();
    if (! Info.bHasSelection)
    { return FString{}; }

    // Cell coordinate + its per-cell tags uniquely identifies what the row list should show. A blocker
    // selection collapses the cell editor, so its tags don't matter to the signature.
    return FString::Printf(TEXT("%d,%d:%s"),
        Info.Coordinate.X, Info.Coordinate.Y, *Info.CellTags.ToStringSimple());
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Rebuild_PerCellTagList() -> void
{
    if (! PerCellTagListContainer.IsValid())
    { return; }

    PerCellTagListContainer->ClearChildren();

    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto Tags  = (Mode != nullptr) ? Mode->Get_SelectedCellTags() : FGameplayTagContainer{};

    if (Tags.IsEmpty())
    {
        PerCellTagListContainer->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock).Text(LOCTEXT("DetailsTagsNone", "(none)"))
        ];
        return;
    }

    for (const auto& Tag : Tags)
    {
        PerCellTagListContainer->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text(FText::FromName(Tag.GetTagName()))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .ToolTipText(LOCTEXT("DetailsRemoveCellTagTip", "Remove this tag from the cell"))
                .OnClicked(this, &FCk_2dGridSystem_EdModeToolkit::On_RemoveCellTag, Tag)
                [
                    SNew(STextBlock).Text(LOCTEXT("DetailsRemoveCellTag", "Remove"))
                ]
            ]
        ];
    }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_AddCellTagChanged(
        const TArray<FGameplayTagContainer>& InContainers) -> void
{
    const auto NewTag = InContainers.IsEmpty() ? FGameplayTag() : InContainers[0].First();
    if (! NewTag.IsValid())
    { return; }

    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return; }

    Mode->Add_SelectedCellTag(NewTag);

    // Reset the picker back to empty so it reads as an "add" control (the row list below is the source of
    // truth for what the cell currently carries). The list rebuild rides the next live-bound repaint.
    if (AddCellTagPicker.IsValid())
    { AddCellTagPicker->SetTagContainers(TArray<FGameplayTagContainer>{ FGameplayTagContainer{} }); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_RemoveCellTag(
        FGameplayTag InTag) -> FReply
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Remove_SelectedCellTag(InTag); }

    return FReply::Handled();
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsBlockerText() const -> FText
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FText::GetEmpty(); }

    const auto Index = Mode->Get_SelectedBlockerIndex();

    // Re-seed the blocker tag picker's displayed value when the selected blocker changes (the picker has
    // no live-bound value attribute). const_cast: this getter is live-bound and owns the view-state seed.
    if (Index != SeededDetailsBlockerIndex)
    {
        auto* MutableThis = const_cast<FCk_2dGridSystem_EdModeToolkit*>(this);
        MutableThis->SeededDetailsBlockerIndex = Index;

        if (DetailsBlockerTagPicker.IsValid())
        {
            auto Container = FGameplayTagContainer{};
            const auto SelectedTag = Mode->Get_SelectedBlockerName();
            if (SelectedTag.IsValid())
            { Container.AddTag(SelectedTag); }

            DetailsBlockerTagPicker->SetTagContainers(TArray<FGameplayTagContainer>{ Container });
        }
    }

    if (Index == INDEX_NONE)
    { return FText::GetEmpty(); }

    auto Min = FIntPoint::ZeroValue;
    auto Max = FIntPoint::ZeroValue;
    if (! Mode->Get_SelectedBlockerRange(Min, Max))
    { return FText::GetEmpty(); }

    const auto SelectedTag = Mode->Get_SelectedBlockerName();
    const auto TagText = SelectedTag.IsValid()
        ? FText::FromName(SelectedTag.GetTagName())
        : LOCTEXT("DetailsBlockerUnnamed2", "(unnamed)");

    return FText::Format(
        LOCTEXT("DetailsBlockerInfo", "Blocker #{0}  range ({1},{2})..({3},{4})  tag: {5}"),
        FText::AsNumber(Index),
        FText::AsNumber(Min.X), FText::AsNumber(Min.Y),
        FText::AsNumber(Max.X), FText::AsNumber(Max.Y),
        TagText);
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_DeleteSelectedBlocker() -> FReply
{
    if (auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get()))
    { Mode->Delete_SelectedBlocker(); }

    return FReply::Handled();
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_DetailsBlockerTagChanged(
        const TArray<FGameplayTagContainer>& InContainers) -> void
{
    const auto NewTag = InContainers.IsEmpty() ? FGameplayTag() : InContainers[0].First();

    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return; }

    // Ignore writes when no blocker is selected (the picker still emits OnTagChanged after a re-seed to
    // the empty container). Mirrors the Blocker section's On_SelectedBlockerTagChanged guard.
    if (Mode->Get_SelectedBlockerIndex() == INDEX_NONE)
    { return; }

    Mode->Set_SelectedBlockerName(NewTag);
    SeededDetailsBlockerIndex = Mode->Get_SelectedBlockerIndex();
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
