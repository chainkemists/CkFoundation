#include "CkUI/Styles/CkCommonButtonStyle_Dropdown.h"

#include "Brushes/SlateColorBrush.h"

UCkCommonButtonStyle_Dropdown::UCkCommonButtonStyle_Dropdown()
{
    const FLinearColor NormalBg(0.14f, 0.15f, 0.16f);
    const FLinearColor HoverBg(0.18f, 0.19f, 0.21f);
    const FLinearColor PressedBg(0.25f, 0.82f, 0.79f);

    //NormalBase.WidgetStyle = FButtonStyle()
    //    .SetNormal(FSlateColorBrush(NormalBg))
    //    .SetHovered(FSlateColorBrush(HoverBg))
    //    .SetPressed(FSlateColorBrush(PressedBg * 0.6f))
    //    .SetNormalForeground(FSlateColor(FLinearColor::White))
    //    .SetHoveredForeground(FSlateColor(FLinearColor::White))
    //    .SetPressedForeground(FSlateColor(FLinearColor::White));

    //// DropDown icon (tiny triangle)
    //NormalBase.DownArrowImage = FSlateBrush();
    //NormalBase.DownArrowImage.ImageSize = FVector2D(8, 8);
    //NormalBase.DownArrowImage.TintColor = FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f));

    //NormalBase.Font = FSlateFontInfo("Roboto", 12);
}
