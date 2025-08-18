#include "CkCommonButtonStyle_Danger.h"

UCkCommonButtonStyle_Danger::UCkCommonButtonStyle_Danger()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.55f, 0.20f, 0.20f)); // Dark red
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.88f, 0.37f, 0.37f)); // Bright red
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.70f, 0.25f, 0.25f));
}
