#include "CkCommonTextStyle_Code.h"

UCkCommonTextStyle_Code::UCkCommonTextStyle_Code()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 11);
    Color = FLinearColor(0.75f, 0.80f, 0.85f); // Slightly bluish-gray
}
