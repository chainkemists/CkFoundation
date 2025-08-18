#include "CkUI/Styles/CkCommonButtonStyle_Tab.h"

UCkCommonButtonStyle_Tab::UCkCommonButtonStyle_Tab()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.14f, 0.15f, 0.16f)); // Gray
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.19f, 0.21f));
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.25f, 0.82f, 0.79f)); // Teal

    //NormalForeground = FSlateColor(FLinearColor(0.62f, 0.64f, 0.67f)); // Secondary text
    //HoveredForeground = FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f));
    //PressedForeground = FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f));
}
