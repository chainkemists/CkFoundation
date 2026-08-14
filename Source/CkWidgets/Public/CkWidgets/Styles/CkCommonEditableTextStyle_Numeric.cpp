#include "CkWidgets/Styles/CkCommonEditableTextStyle_Numeric.h"

#include "Brushes/SlateColorBrush.h"

UCkCommonEditableTextStyle_Numeric::UCkCommonEditableTextStyle_Numeric()
{
    // Monospaced digits for price/quantity entry
    Font = FCoreStyle::GetDefaultFontStyle("Mono", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f); // Light gray text

    const FLinearColor PanelColor(0.14f, 0.15f, 0.16f);
    const FLinearColor BorderColor(0.18f, 0.19f, 0.21f);
    const FLinearColor FocusedColor(0.25f, 0.82f, 0.79f);
    const FLinearColor DisabledColor(0.42f, 0.44f, 0.45f);

    EditableTextBoxStyle = FEditableTextBoxStyle()
        .SetFont(Font)
        .SetForegroundColor(Color)
        .SetBackgroundImageNormal(FSlateColorBrush(PanelColor))
        .SetBackgroundImageHovered(FSlateColorBrush(BorderColor))
        .SetBackgroundImageFocused(FSlateColorBrush(FocusedColor * 0.4f))
        .SetBackgroundImageReadOnly(FSlateColorBrush(PanelColor * 0.8f))
        .SetFocusedForegroundColor(Color)
        .SetReadOnlyForegroundColor(DisabledColor);
}
