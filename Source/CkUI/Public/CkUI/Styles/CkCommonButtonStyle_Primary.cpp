
#include "CkUI/Styles/CkCommonButtonStyle_Primary.h"

UCkCommonButtonStyle_Primary::UCkCommonButtonStyle_Primary()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.25f, 0.82f, 0.79f)); // Teal
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.30f, 0.90f, 0.87f));
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.20f, 0.70f, 0.68f));

    //NormalForeground = FSlateColor(FLinearColor::Black);
    //HoveredForeground = FSlateColor(FLinearColor::Black);
    //PressedForeground = FSlateColor(FLinearColor::Black);
}
