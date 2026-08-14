#include "CkWidgets/Styles/CkCommonButtonStyle_IconOnly.h"

UCkCommonButtonStyle_IconOnly::UCkCommonButtonStyle_IconOnly()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)); // Icon carries the affordance
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.19f, 0.21f)); // Border gray
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.25f, 0.82f, 0.79f, 0.35f)); // Teal
}
