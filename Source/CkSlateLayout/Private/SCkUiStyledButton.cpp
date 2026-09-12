#include "CkSlateLayout/SCkUiStyledButton.h"

#include "Brushes/SlateRoundedBoxBrush.h"

namespace ck_ui_styled_button
{
    const FLinearColor DefaultBackground(0x1A / 255.0f, 0x2A / 255.0f, 0x38 / 255.0f, 1.0f);
    const FLinearColor DefaultBorder(0x31 / 255.0f, 0x49 / 255.0f, 0x5B / 255.0f, 1.0f);
    const FLinearColor DefaultHoverBackground(0x22 / 255.0f, 0x37 / 255.0f, 0x46 / 255.0f, 1.0f);
    const FLinearColor DefaultHoverBorder(0x48 / 255.0f, 0x70 / 255.0f, 0x84 / 255.0f, 1.0f);
    const FLinearColor DefaultPressedBackground(0x1C / 255.0f, 0x30 / 255.0f, 0x3E / 255.0f, 1.0f);
    const FLinearColor DefaultPressedBorder(0x3D / 255.0f, 0x63 / 255.0f, 0x76 / 255.0f, 1.0f);
    const FLinearColor DefaultForeground(0xE9 / 255.0f, 0xF1 / 255.0f, 0xF7 / 255.0f, 1.0f);
    const FLinearColor DefaultDisabledForeground(0x90 / 255.0f, 0xA4 / 255.0f, 0xB5 / 255.0f, 0.55f);

    auto ReadColor(const TOptional<FLinearColor>& InValue, const FLinearColor& InDefault) -> FLinearColor
    {
        return InValue.Get(InDefault);
    }

    auto MakeDisabledColor(const FLinearColor& InColor) -> FLinearColor
    {
        auto Result = InColor;
        Result.A *= 0.55f;
        return Result;
    }

}

void SCkUiStyledButton::Construct(const FArguments& InArgs)
{
    _ButtonStyle = MakeButtonStyle(InArgs._VisualStyle);
    SButton::Construct(SButton::FArguments()
        .ButtonStyle(&_ButtonStyle)
        .ContentPadding(InArgs._VisualStyle.ContentPadding)
        .OnClicked(InArgs._OnClicked)
        [
            InArgs._Content.Widget
        ]);
    // This class calls SButton::Construct directly, so inherited SWidget
    // arguments are not applied by the usual SNew declarative wrapper.
    SetEnabled(InArgs._IsEnabled);
    SetToolTipText(InArgs._ToolTipText);
    SetTag(InArgs._Tag);
}

auto SCkUiStyledButton::MakeButtonStyle(const FCkUiButtonVisualStyle& InVisualStyle) -> FButtonStyle
{
    using namespace ck_ui_styled_button;

    const FLinearColor Background = ReadColor(InVisualStyle.Background, DefaultBackground);
    const FLinearColor Border = ReadColor(InVisualStyle.BorderColor, DefaultBorder);
    const FLinearColor HoverBackground = ReadColor(InVisualStyle.HoverBackground, DefaultHoverBackground);
    const FLinearColor HoverBorder = ReadColor(InVisualStyle.HoverBorderColor, DefaultHoverBorder);
    const FLinearColor PressedBackground = ReadColor(InVisualStyle.PressedBackground, DefaultPressedBackground);
    const FLinearColor PressedBorder = ReadColor(InVisualStyle.PressedBorderColor, DefaultPressedBorder);
    const FLinearColor DisabledBackground = ReadColor(InVisualStyle.DisabledBackground, MakeDisabledColor(Background));
    const FLinearColor DisabledBorder = ReadColor(InVisualStyle.DisabledBorderColor, MakeDisabledColor(Border));
    const FLinearColor Foreground = ReadColor(InVisualStyle.Color, DefaultForeground);
    const FLinearColor DisabledForeground = ReadColor(InVisualStyle.DisabledColor, DefaultDisabledForeground);
    const float Radius = InVisualStyle.Radius.Get(4.0f);
    const float OutlineWidth = InVisualStyle.OutlineWidth.Get(1.0f);

    return FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(Background, Radius, Border, OutlineWidth))
        .SetHovered(FSlateRoundedBoxBrush(HoverBackground, Radius, HoverBorder, OutlineWidth))
        .SetPressed(FSlateRoundedBoxBrush(PressedBackground, Radius, PressedBorder, OutlineWidth))
        .SetDisabled(FSlateRoundedBoxBrush(DisabledBackground, Radius, DisabledBorder, OutlineWidth))
        .SetNormalForeground(FSlateColor(Foreground))
        .SetHoveredForeground(FSlateColor(Foreground))
        .SetPressedForeground(FSlateColor(Foreground))
        .SetDisabledForeground(FSlateColor(DisabledForeground))
        .SetNormalPadding(FMargin(0.0f))
        .SetPressedPadding(FMargin(0.0f));
}
