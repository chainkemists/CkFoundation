#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"

#include "Widgets/Layout/SBorder.h"
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
    // Placeholder palette panel — the paint tools palette replaces this in a later task.
    SAssignNew(InlineContent, SBorder)
    .Padding(8.0f)
    [
        SNew(STextBlock)
        .Text(LOCTEXT("PaintPanelPlaceholder", "Grid Paint"))
    ];

    FModeToolkit::Init(InToolkitHost, InOwningMode);
}

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
