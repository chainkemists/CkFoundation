#include "CkCommonTextStyle_Secondary.h"

UCkCommonTextStyle_Secondary::UCkCommonTextStyle_Secondary()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.62f, 0.64f, 0.67f); // Muted gray
}
