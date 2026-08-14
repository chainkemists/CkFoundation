#include "CkWidgets/Styles/CkCommonTextStyle_Caption.h"

UCkCommonTextStyle_Caption::UCkCommonTextStyle_Caption()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 10);
    Color = FLinearColor(0.62f, 0.64f, 0.67f); // Muted gray
}
