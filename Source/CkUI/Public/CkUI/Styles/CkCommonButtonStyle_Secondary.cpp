#include "CkCommonButtonStyle_Secondary.h"

UCkCommonButtonStyle_Secondary::UCkCommonButtonStyle_Secondary()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.14f, 0.15f, 0.16f)); // Panel gray
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.18f, 0.19f, 0.21f)); // Border gray
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.20f, 0.22f, 0.24f));
}
