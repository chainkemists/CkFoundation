#include "CkWidgets/Styles/CkCommonButtonStyle_Link.h"

UCkCommonButtonStyle_Link::UCkCommonButtonStyle_Link()
{
    NormalBase.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)); // Text-only affordance
    NormalHovered.TintColor = FSlateColor(FLinearColor(0.25f, 0.82f, 0.79f, 0.10f)); // Faint teal underlay
    NormalPressed.TintColor = FSlateColor(FLinearColor(0.25f, 0.82f, 0.79f, 0.20f));
}
