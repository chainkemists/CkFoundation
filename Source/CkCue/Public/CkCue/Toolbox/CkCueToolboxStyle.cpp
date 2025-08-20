#include "CkCueToolboxStyle.h"

#include <Styling/SlateStyleMacros.h>
#include <Styling/CoreStyle.h>
#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

TSharedPtr<FSlateStyleSet> FCkCueToolboxStyle::StyleInstance = nullptr;

// --------------------------------------------------------------------------------------------------------------------

auto FCkCueToolboxStyle::Initialize() -> void
{
    if (NOT StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

auto FCkCueToolboxStyle::Shutdown() -> void
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

auto FCkCueToolboxStyle::ReloadTextures() -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

auto FCkCueToolboxStyle::Get_StyleSetName() -> FName
{
    static FName StyleSetName(TEXT("CkCueToolboxStyle"));
    return StyleSetName;
}

auto FCkCueToolboxStyle::Create() -> TSharedRef<FSlateStyleSet>
{
    auto Style = MakeShared<FSlateStyleSet>(Get_StyleSetName());

    // Try to find the plugin, but don't crash if it doesn't exist
    const auto Plugin = IPluginManager::Get().FindPlugin("CkFoundation");
    if (Plugin.IsValid())
    {
        Style->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    }
    else
    {
        // Fallback to engine content
        Style->SetContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
    }

    const auto NormalGray = FLinearColor(0.016f, 0.016f, 0.016f);
    const auto LightGray = FLinearColor(0.035f, 0.035f, 0.035f);
    const auto DarkGray = FLinearColor(0.008f, 0.008f, 0.008f);
    const auto AccentBlue = FLinearColor(0.25f, 0.82f, 0.79f);
    const auto WarningOrange = FLinearColor(0.88f, 0.37f, 0.37f);
    const auto DebugPurple = FLinearColor(0.61f, 0.37f, 0.88f);
    const auto TextWhite = FLinearColor(0.9f, 0.9f, 0.9f);
    const auto TextGray = FLinearColor(0.6f, 0.6f, 0.6f);

    Style->Set("CkCueToolbox.Color.Primary", TextWhite);
    Style->Set("CkCueToolbox.Color.Secondary", TextGray);
    Style->Set("CkCueToolbox.Color.Accent", AccentBlue);
    Style->Set("CkCueToolbox.Color.Warning", WarningOrange);
    Style->Set("CkCueToolbox.Color.Debug", DebugPurple);

    const auto DefaultFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    const auto BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);
    const auto HeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);

    Style->Set("CkCueToolbox.Font.Regular", DefaultFont);
    Style->Set("CkCueToolbox.Font.Bold", BoldFont);
    Style->Set("CkCueToolbox.Font.Header", HeaderFont);

    const auto ButtonNormal = FButtonStyle()
        .SetNormal(FSlateColorBrush(LightGray))
        .SetHovered(FSlateColorBrush(AccentBlue))
        .SetPressed(FSlateColorBrush(AccentBlue * 0.8f))
        .SetDisabled(FSlateColorBrush(DarkGray))
        .SetNormalPadding(FMargin(8, 4))
        .SetPressedPadding(FMargin(8, 4));

    Style->Set("CkCueToolbox.Button.Primary", ButtonNormal);

    const auto ButtonSecondary = FButtonStyle(ButtonNormal)
        .SetNormal(FSlateColorBrush(NormalGray))
        .SetHovered(FSlateColorBrush(LightGray))
        .SetPressed(FSlateColorBrush(DarkGray));

    Style->Set("CkCueToolbox.Button.Secondary", ButtonSecondary);

    const auto ButtonSmall = FButtonStyle(ButtonNormal)
        .SetNormalPadding(FMargin(4, 2))
        .SetPressedPadding(FMargin(4, 2));

    Style->Set("CkCueToolbox.Button.Small", ButtonSmall);

    const auto TableRowNormal = FTableRowStyle()
        .SetEvenRowBackgroundBrush(FSlateColorBrush(NormalGray))
        .SetOddRowBackgroundBrush(FSlateColorBrush(LightGray))
        .SetSelectorFocusedBrush(FSlateColorBrush(AccentBlue))
        .SetActiveBrush(FSlateColorBrush(AccentBlue * 0.6f))
        .SetActiveHoveredBrush(FSlateColorBrush(AccentBlue * 0.8f))
        .SetInactiveBrush(FSlateColorBrush(LightGray))
        .SetInactiveHoveredBrush(FSlateColorBrush(LightGray * 1.2f))
        .SetTextColor(TextWhite)
        .SetSelectedTextColor(FLinearColor::Black);

    Style->Set("CkCueToolbox.TableRow.Normal", TableRowNormal);

    const auto HeaderRowStyle = FHeaderRowStyle()
        .SetBackgroundBrush(FSlateColorBrush(DarkGray))
        .SetForegroundColor(TextWhite);

    Style->Set("CkCueToolbox.HeaderRow.Normal", HeaderRowStyle);

    const auto SearchBoxStyle = FSearchBoxStyle()
        .SetTextBoxStyle(FEditableTextBoxStyle()
            .SetBackgroundImageNormal(FSlateColorBrush(LightGray))
            .SetBackgroundImageHovered(FSlateColorBrush(LightGray * 1.1f))
            .SetBackgroundImageFocused(FSlateColorBrush(LightGray * 1.2f))
            .SetBackgroundImageReadOnly(FSlateColorBrush(DarkGray))
            .SetPadding(FMargin(4.0f))
            .SetFont(DefaultFont)
            .SetForegroundColor(TextWhite))
        .SetUpArrowImage(FSlateNoResource())
        .SetDownArrowImage(FSlateNoResource())
        .SetGlassImage(FSlateNoResource())
        .SetClearImage(FSlateNoResource());

    Style->Set("CkCueToolbox.SearchBox.Normal", SearchBoxStyle);

    const auto EditableTextBoxStyle = FEditableTextBoxStyle()
        .SetBackgroundImageNormal(FSlateColorBrush(LightGray))
        .SetBackgroundImageHovered(FSlateColorBrush(LightGray * 1.1f))
        .SetBackgroundImageFocused(FSlateColorBrush(LightGray * 1.2f))
        .SetBackgroundImageReadOnly(FSlateColorBrush(DarkGray))
        .SetPadding(FMargin(4.0f))
        .SetFont(DefaultFont)
        .SetForegroundColor(TextWhite)
        .SetBackgroundColor(FLinearColor::Transparent)
        .SetReadOnlyForegroundColor(TextGray);

    Style->Set("CkCueToolbox.EditableTextBox.Normal", EditableTextBoxStyle);

    const auto ComboBoxStyle = FComboBoxStyle()
        .SetComboButtonStyle(FComboButtonStyle()
            .SetButtonStyle(FButtonStyle()
                .SetNormal(FSlateColorBrush(LightGray))
                .SetHovered(FSlateColorBrush(LightGray * 1.1f))
                .SetPressed(FSlateColorBrush(LightGray * 0.9f))
                .SetDisabled(FSlateColorBrush(DarkGray))
                .SetNormalPadding(FMargin(8, 4))
                .SetPressedPadding(FMargin(8, 4)))
            .SetDownArrowImage(FSlateNoResource())
            .SetMenuBorderBrush(FSlateColorBrush(DarkGray))
            .SetMenuBorderPadding(FMargin(1.0f)))
        .SetMenuRowPadding(FMargin(8, 2));

    Style->Set("CkCueToolbox.ComboBox.Normal", ComboBoxStyle);

    const auto ScrollBarStyle = FScrollBarStyle()
        .SetNormalThumbImage(FSlateColorBrush(LightGray))
        .SetHoveredThumbImage(FSlateColorBrush(AccentBlue))
        .SetDraggedThumbImage(FSlateColorBrush(AccentBlue * 0.8f));

    const auto ScrollBoxStyle = FScrollBoxStyle()
        .SetTopShadowBrush(FSlateNoResource())
        .SetBottomShadowBrush(FSlateNoResource())
        .SetLeftShadowBrush(FSlateNoResource())
        .SetRightShadowBrush(FSlateNoResource());

    Style->Set("CkCueToolbox.ScrollBox.Normal", ScrollBoxStyle);
    Style->Set("CkCueToolbox.ScrollBar.Normal", ScrollBarStyle);

    return Style;
}

// --------------------------------------------------------------------------------------------------------------------