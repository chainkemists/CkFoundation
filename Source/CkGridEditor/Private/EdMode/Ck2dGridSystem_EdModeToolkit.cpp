#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"
#include "CkGridEditor/EdMode/Ck2dGridSystem_ToolkitWidgets.h"
#include "CkGridEditor/Draw/Ck2dGridSystem_AuthoredOverlay.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "SGameplayTagPicker.h"

#include "Styling/AppStyle.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

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

    namespace style = ck::grid_paint_style;

    SAssignNew(InlineContent, SBorder)
    .BorderImage(style::WhiteBrush())
    .BorderBackgroundColor(FSlateColor(style::BgRoot()))
    .Padding(FMargin(style::SpaceM))
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .Padding(0.0f, 0.0f, 0.0f, style::SpaceM)
        [
            Build_GridSection()
        ]
        + SScrollBox::Slot()
        [
            style::Make_SectionHeader(LOCTEXT("PaintToolsLabel", "Paint Tool"))
        ]
        + SScrollBox::Slot()
        .Padding(0.0f, 0.0f, 0.0f, style::SpaceM)
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
        + SScrollBox::Slot()
        [
            Build_TagsSection()
        ]
        + SScrollBox::Slot()
        [
            Build_BlockerSection()
        ]
        + SScrollBox::Slot()
        [
            Build_DetailsSection()
        ]
    ];

    FModeToolkit::Init(InToolkitHost, InOwningMode);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Build_GridSection() -> TSharedRef<SWidget>
{
    namespace style = ck::grid_paint_style;

    const auto MakeAxisLabel = [](const FText& InLabel) -> TSharedRef<SWidget>
    {
        return SNew(STextBlock)
            .Text(InLabel)
            .Font(style::Font_Regular(9))
            .ColorAndOpacity(FSlateColor(style::TextMute()));
    };

    auto Body = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, style::SpaceS)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
            [
                MakeAxisLabel(LOCTEXT("GridDimensions", "Dimensions"))
            ]
            + SHorizontalBox::Slot().FillWidth(0.29f).Padding(style::SpaceXS, 0.0f)
            [
                SNew(SNumericEntryBox<int32>)
                .Value(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridDimensionX)
                .OnValueCommitted(this, &FCk_2dGridSystem_EdModeToolkit::On_GridDimensionXCommitted)
                .AllowSpin(true)
                .MinValue(1)
                .MinSliderValue(1)
                .MaxSliderValue(64)
            ]
            + SHorizontalBox::Slot().FillWidth(0.29f).Padding(style::SpaceXS, 0.0f)
            [
                SNew(SNumericEntryBox<int32>)
                .Value(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridDimensionY)
                .OnValueCommitted(this, &FCk_2dGridSystem_EdModeToolkit::On_GridDimensionYCommitted)
                .AllowSpin(true)
                .MinValue(1)
                .MinSliderValue(1)
                .MaxSliderValue(64)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
            [
                MakeAxisLabel(LOCTEXT("GridCellSize", "Cell Size"))
            ]
            + SHorizontalBox::Slot().FillWidth(0.29f).Padding(style::SpaceXS, 0.0f)
            [
                SNew(SNumericEntryBox<double>)
                .Value(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridCellSizeX)
                .OnValueCommitted(this, &FCk_2dGridSystem_EdModeToolkit::On_GridCellSizeXCommitted)
                .AllowSpin(true)
                .MinValue(1.0)
                .MinSliderValue(1.0)
                .MaxSliderValue(1000.0)
            ]
            + SHorizontalBox::Slot().FillWidth(0.29f).Padding(style::SpaceXS, 0.0f)
            [
                SNew(SNumericEntryBox<double>)
                .Value(this, &FCk_2dGridSystem_EdModeToolkit::Get_GridCellSizeY)
                .OnValueCommitted(this, &FCk_2dGridSystem_EdModeToolkit::On_GridCellSizeYCommitted)
                .AllowSpin(true)
                .MinValue(1.0)
                .MinSliderValue(1.0)
                .MaxSliderValue(1000.0)
            ]
        ];

    return style::Make_LabeledGroup(LOCTEXT("GridSectionTitle", "Grid"), Body);
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_GridDimensionX() const -> TOptional<int32>
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec == nullptr)
    { return TOptional<int32>{}; }
    return Spec->Dimensions.X;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_GridDimensionY() const -> TOptional<int32>
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec == nullptr)
    { return TOptional<int32>{}; }
    return Spec->Dimensions.Y;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_GridCellSizeX() const -> TOptional<double>
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec == nullptr)
    { return TOptional<double>{}; }
    return Spec->CellSize.X;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_GridCellSizeY() const -> TOptional<double>
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec == nullptr)
    { return TOptional<double>{}; }
    return Spec->CellSize.Y;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_GridDimensionXCommitted(
        int32             InValue,
        ETextCommit::Type InCommitType) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Mode == nullptr || Spec == nullptr)
    { return; }
    Mode->Set_GridDimensions(FIntPoint(InValue, Spec->Dimensions.Y));
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_GridDimensionYCommitted(
        int32             InValue,
        ETextCommit::Type InCommitType) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Mode == nullptr || Spec == nullptr)
    { return; }
    Mode->Set_GridDimensions(FIntPoint(Spec->Dimensions.X, InValue));
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_GridCellSizeXCommitted(
        double            InValue,
        ETextCommit::Type InCommitType) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Mode == nullptr || Spec == nullptr)
    { return; }
    Mode->Set_GridCellSize(FVector2D(InValue, Spec->CellSize.Y));
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_GridCellSizeYCommitted(
        double            InValue,
        ETextCommit::Type InCommitType) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Mode == nullptr || Spec == nullptr)
    { return; }
    Mode->Set_GridCellSize(FVector2D(Spec->CellSize.X, InValue));
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

    // Read-only per-tag color legend (swatch + name + count), rebuilt from the Spec on repaint.
    SAssignNew(TagLegendContainer, SVerticalBox);
    Rebuild_TagLegend();

    namespace style = ck::grid_paint_style;

    auto Body = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            style::Make_SectionHeader(LOCTEXT("PaintTagLabel", "Paint Tag"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            TagPicker.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, style::SpaceM, 0.0f, 0.0f)
        [
            style::Make_SectionHeader(LOCTEXT("TagColorsLabel", "Tag Colors"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            TagLegendContainer.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, style::SpaceM, 0.0f, 0.0f)
        [
            style::Make_SectionHeader(LOCTEXT("TagScopeLabel", "Scope"))
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
        .Padding(0.0f, style::SpaceM, 0.0f, 0.0f)
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
        ];

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_TagsSectionVisibility)
        [
            style::Make_LabeledGroup(LOCTEXT("TagsSectionTitle", "Tags"), Body)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Compute_TagLegendSignature() const -> FString
{
    const auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr)
    { return FString{}; }

    const auto* Spec = Mode->Get_SelectedSpec();
    if (Spec == nullptr)
    { return FString{}; }

    auto Sig = FString{};
    for (const auto& Pair : ck::grid_editor::Collect_PerCellTagsWithCounts(Spec))
    { Sig += Pair.Key.GetTagName().ToString() + FString::Printf(TEXT(":%d;"), Pair.Value); }
    return Sig;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Rebuild_TagLegend() -> void
{
    if (! TagLegendContainer.IsValid())
    { return; }

    TagLegendContainer->ClearChildren();

    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    const auto Entries = Spec != nullptr
        ? ck::grid_editor::Collect_PerCellTagsWithCounts(Spec)
        : TArray<TPair<FGameplayTag, int32>>{};

    namespace style = ck::grid_paint_style;

    if (Entries.Num() == 0)
    {
        TagLegendContainer->AddSlot().AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("LegendNone", "(no per-cell tags)"))
            .Font(style::Font_Regular(9))
            .ColorAndOpacity(FSlateColor(style::TextMute()))
        ];
        return;
    }

    for (const auto& Entry : Entries)
    {
        const auto Color = ck::grid_editor::Resolve_TagColor(Entry.Key);
        const auto Name  = FText::FromString(Entry.Key.GetTagName().ToString());

        TagLegendContainer->AddSlot().AutoHeight().Padding(0.0f, 1.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, style::SpaceM, 0.0f)
            [
                style::Make_Swatch(Color, 12.0f)
            ]
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(Name)
                .Font(style::Font_Regular(9))
                .ColorAndOpacity(FSlateColor(style::Text()))
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                style::Make_CountBadge(Entry.Value)
            ]
        ];
    }
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

    namespace style = ck::grid_paint_style;

    auto Body = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            style::Make_SectionHeader(LOCTEXT("NewBlockerTagLabel", "New Blocker Tag"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            NewBlockerTagPicker.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, style::SpaceM, 0.0f, style::SpaceXS)
        [
            SNew(STextBlock)
            .Text(this, &FCk_2dGridSystem_EdModeToolkit::Get_SelectedBlockerText)
            .Font(style::Font_Regular(9))
            .ColorAndOpacity(FSlateColor(style::TextDim()))
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
                .Padding(0.0f, style::SpaceS, 0.0f, 0.0f)
                [
                    style::Make_SectionHeader(LOCTEXT("SelectedBlockerTagLabel", "Selected Blocker Tag"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SelectedBlockerTagPicker.ToSharedRef()
                ]
            ]
        ];

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_BlockerSectionVisibility)
        [
            style::Make_LabeledGroup(LOCTEXT("BlockerSectionTitle", "Blocker"), Body)
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
    namespace style = ck::grid_paint_style;

    // Live-bound key/value row helper (debugger-style). Value text reads the EdMode's selected-cell info
    // off the Spec each frame, so it tracks live edits to the Spec.
    const auto MakeRow = [](const FText& InLabel, TAttribute<FText> InValue) -> TSharedRef<SWidget>
    {
        return style::Make_KeyValueRow(InLabel, InValue);
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
            .Padding(0.0f, style::SpaceS, 0.0f, 0.0f)
            [
                style::Make_SectionHeader(LOCTEXT("DetailsCellTagsLabel", "Cell Tags"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                PerCellTagListContainer.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, style::SpaceS, 0.0f, 0.0f)
            [
                style::Make_SectionHeader(LOCTEXT("DetailsAddCellTagLabel", "Add Cell Tag"))
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

    SAssignNew(BlockerListView, SListView<TSharedPtr<FCk_GridBlockerListItem>>)
        .ListItemsSource(&BlockerListItems)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &FCk_2dGridSystem_EdModeToolkit::OnGenerate_BlockerRow)
        .OnSelectionChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_BlockerRowSelected);

    SAssignNew(TagListView, SListView<TSharedPtr<FCk_GridTagListItem>>)
        .ListItemsSource(&TagListItems)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow(this, &FCk_2dGridSystem_EdModeToolkit::OnGenerate_TagRow)
        .OnSelectionChanged(this, &FCk_2dGridSystem_EdModeToolkit::On_TagRowSelected);

    Rebuild_SelectLists();

    auto Body = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            style::Make_SectionHeader(LOCTEXT("BlockersListLabel", "Blockers"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .MaxHeight(140.0f)
        [
            BlockerListView.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, style::SpaceM, 0.0f, 0.0f)
        [
            style::Make_SectionHeader(LOCTEXT("TagsListLabel", "Tags"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .MaxHeight(140.0f)
        [
            TagListView.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, style::SpaceM, 0.0f, 0.0f)
        [
            CellEditor
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            BlockerEditor
        ];

    return SNew(SBox)
        .Visibility(this, &FCk_2dGridSystem_EdModeToolkit::Get_DetailsSectionVisibility)
        [
            style::Make_LabeledGroup(LOCTEXT("DetailsLabel", "Cell Details"), Body)
        ];
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Get_DetailsSectionVisibility() const -> EVisibility
{
    // Re-drive the Blockers/Tags lists on repaint when the Spec's blocker/tag set changes (mirrors the
    // const-cast-in-getter refresh idiom). The signature guard keeps it cheap.
    if (const auto Sig = Compute_SelectListsSignature(); Sig != SeededSelectListsSignature)
    {
        const_cast<FCk_2dGridSystem_EdModeToolkit*>(this)->SeededSelectListsSignature = Sig;
        const_cast<FCk_2dGridSystem_EdModeToolkit*>(this)->Rebuild_SelectLists();
    }

    return Get_ActiveTool() == ECk_GridPaint_Tool::Select
        ? EVisibility::Visible
        : EVisibility::Collapsed;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_EdModeToolkit::
    Compute_SelectListsSignature() const -> FString
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec == nullptr)
    { return FString{}; }

    // Include each blocker's name + range so a rename / re-range re-drives the Blockers list row labels
    // (count alone misses an in-place edit via the blocker tag picker).
    auto Sig = FString::Printf(TEXT("B%d|"), Spec->Blockers.Num());
    for (const auto& Blocker : Spec->Blockers)
    {
        Sig += FString::Printf(TEXT("%s@%d,%d:%d,%d;"),
            *Blocker.Name.GetTagName().ToString(),
            Blocker.RangeMin.X, Blocker.RangeMin.Y, Blocker.RangeMax.X, Blocker.RangeMax.Y);
    }
    Sig += TEXT("|");
    for (const auto& Pair : ck::grid_editor::Collect_PerCellTagsWithCounts(Spec))
    { Sig += FString::Printf(TEXT("%s:%d;"), *Pair.Key.GetTagName().ToString(), Pair.Value); }
    return Sig;
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    Rebuild_SelectLists() -> void
{
    BlockerListItems.Reset();
    TagListItems.Reset();

    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    const auto* Spec = Mode != nullptr ? Mode->Get_SelectedSpec() : nullptr;
    if (Spec != nullptr)
    {
        for (auto Index = 0; Index < Spec->Blockers.Num(); ++Index)
        {
            const auto& B = Spec->Blockers[Index];
            auto Item      = MakeShared<FCk_GridBlockerListItem>();
            Item->Index    = Index;
            Item->Name     = B.Name;
            Item->RangeMin = B.RangeMin;
            Item->RangeMax = B.RangeMax;
            BlockerListItems.Add(Item);
        }

        for (const auto& Pair : ck::grid_editor::Collect_PerCellTagsWithCounts(Spec))
        {
            auto Item       = MakeShared<FCk_GridTagListItem>();
            Item->Tag       = Pair.Key;
            Item->CellCount = Pair.Value;
            TagListItems.Add(Item);
        }
    }

    if (BlockerListView.IsValid())
    { BlockerListView->RequestListRefresh(); }
    if (TagListView.IsValid())
    { TagListView->RequestListRefresh(); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    OnGenerate_BlockerRow(
        TSharedPtr<FCk_GridBlockerListItem> InItem,
        const TSharedRef<STableViewBase>&   InOwner) -> TSharedRef<ITableRow>
{
    namespace style = ck::grid_paint_style;

    const auto NameStr = InItem->Name.IsValid() ? InItem->Name.GetTagName().ToString() : FString(TEXT("(anon)"));
    const auto Label = FText::FromString(FString::Printf(
        TEXT("#%d  %s  (%d,%d)..(%d,%d)"),
        InItem->Index, *NameStr,
        InItem->RangeMin.X, InItem->RangeMin.Y, InItem->RangeMax.X, InItem->RangeMax.Y));

    return SNew(STableRow<TSharedPtr<FCk_GridBlockerListItem>>, InOwner)
        .Padding(FMargin(0.0f, 1.0f))
        .ShowSelection(true)
    [
        SNew(STextBlock)
        .Text(Label)
        .Font(style::Font_Regular(9))
        .ColorAndOpacity(FSlateColor(style::Text()))
    ];
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    OnGenerate_TagRow(
        TSharedPtr<FCk_GridTagListItem>   InItem,
        const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>
{
    namespace style = ck::grid_paint_style;

    const auto Color = ck::grid_editor::Resolve_TagColor(InItem->Tag);
    const auto Name  = FText::FromString(InItem->Tag.GetTagName().ToString());

    return SNew(STableRow<TSharedPtr<FCk_GridTagListItem>>, InOwner)
        .Padding(FMargin(0.0f, 1.0f))
        .ShowSelection(true)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, style::SpaceM, 0.0f)
        [
            style::Make_Swatch(Color, 12.0f)
        ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text(Name)
            .Font(style::Font_Regular(9))
            .ColorAndOpacity(FSlateColor(style::Text()))
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            style::Make_CountBadge(InItem->CellCount)
        ]
    ];
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_BlockerRowSelected(
        TSharedPtr<FCk_GridBlockerListItem> InItem,
        ESelectInfo::Type                   InSelectInfo) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr || ! InItem.IsValid())
    { return; }

    Mode->Set_SelectedBlockerIndex(InItem->Index);

    // Clear the Tags list's visual selection so the two lists don't both look active.
    if (TagListView.IsValid())
    { TagListView->ClearSelection(); }
}

auto
    FCk_2dGridSystem_EdModeToolkit::
    On_TagRowSelected(
        TSharedPtr<FCk_GridTagListItem> InItem,
        ESelectInfo::Type               InSelectInfo) -> void
{
    auto* Mode = Cast<UCk_2dGridSystem_EdMode>(OwningMode.Get());
    if (Mode == nullptr || ! InItem.IsValid())
    { return; }

    Mode->Set_SelectedTag(TOptional<FGameplayTag>(InItem->Tag));

    if (BlockerListView.IsValid())
    { BlockerListView->ClearSelection(); }
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
    // Re-drive the legend on repaint when the per-cell tag set changes (mirrors the const-cast-in-getter
    // refresh idiom used by Get_DetailsCoordinateText). The signature guard keeps it cheap.
    if (const auto Sig = Compute_TagLegendSignature(); Sig != SeededTagLegendSignature)
    {
        const_cast<FCk_2dGridSystem_EdModeToolkit*>(this)->SeededTagLegendSignature = Sig;
        const_cast<FCk_2dGridSystem_EdModeToolkit*>(this)->Rebuild_TagLegend();
    }

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
