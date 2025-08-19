#include "CkUI/Styles/CkCommonTextStyle_Dropdown.h"

UCkCommonTextStyle_Dropdown::UCkCommonTextStyle_Dropdown()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f);
}
