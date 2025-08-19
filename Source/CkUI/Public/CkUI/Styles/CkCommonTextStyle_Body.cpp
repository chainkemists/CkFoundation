
#include "CkUI/Styles/CkCommonTextStyle_Body.h"

UCkCommonTextStyle_Body::UCkCommonTextStyle_Body()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f); // Default light gray
}
