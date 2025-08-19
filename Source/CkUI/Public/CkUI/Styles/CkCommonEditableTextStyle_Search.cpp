#include "CkUI/Styles/CkCommonEditableTextStyle_Search.h"

#include "Brushes/SlateColorBrush.h"

UCkCommonEditableTextStyle_Search::UCkCommonEditableTextStyle_Search()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f);

    const FLinearColor PanelColor(0.14f, 0.15f, 0.16f);
    const FLinearColor BorderColor(0.18f, 0.19f, 0.21f);
    const FLinearColor FocusedColor(0.25f, 0.82f, 0.79f);

    EditableTextBoxStyle = FEditableTextBoxStyle()
        .SetFont(Font)
        .SetForegroundColor(Color)
        .SetBackgroundImageNormal(FSlateColorBrush(PanelColor))
        .SetBackgroundImageHovered(FSlateColorBrush(BorderColor))
        .SetBackgroundImageFocused(FSlateColorBrush(FocusedColor * 0.4f))
        .SetBackgroundImageReadOnly(FSlateColorBrush(PanelColor * 0.8f))
        .SetFocusedForegroundColor(Color)
        .SetReadOnlyForegroundColor(FSlateColor(FLinearColor(0.42f, 0.44f, 0.45f)));
}
