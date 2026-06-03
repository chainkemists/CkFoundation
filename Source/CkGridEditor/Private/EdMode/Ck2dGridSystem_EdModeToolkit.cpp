#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "Styling/AppStyle.h"

#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
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
        ]
    ];

    FModeToolkit::Init(InToolkitHost, InOwningMode);
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
