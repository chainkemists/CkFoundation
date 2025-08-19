#include "CkUI/Styles/CkCommonTextStyle_Header.h"

UCkCommonTextStyle_Header::UCkCommonTextStyle_Header()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 16);
    Color = FLinearColor(0.85f, 0.85f, 0.85f); // Light gray
}
