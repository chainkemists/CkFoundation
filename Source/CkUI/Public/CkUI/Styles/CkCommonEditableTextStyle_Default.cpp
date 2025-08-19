#include "CkCommonEditableTextStyle_Default.h"

UCkCommonEditableTextStyle_Default::UCkCommonEditableTextStyle_Default()
{
    // Base font & text color
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f); // Light gray text

    // Color palette
    const FLinearColor PanelColor(0.14f, 0.15f, 0.16f);    // #232529
    const FLinearColor BorderColor(0.18f, 0.19f, 0.21f);   // #2E3136
    const FLinearColor FocusedColor(0.25f, 0.82f, 0.79f);  // Teal
    const FLinearColor DisabledColor(0.42f, 0.44f, 0.45f); // Muted gray

    // EditableTextBox style
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
