#include "CkUI/Styles/CkCommonButtonStyle_Ghost.h"

UCkCommonButtonStyle_Ghost::UCkCommonButtonStyle_Ghost()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)); // Invisible until hovered
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.19f, 0.21f)); // Border gray
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.20f, 0.22f, 0.24f));
}
