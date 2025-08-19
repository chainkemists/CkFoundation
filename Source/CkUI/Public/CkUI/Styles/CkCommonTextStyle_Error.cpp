#include "CkUI/Styles/CkCommonTextStyle_Error.h"

UCkCommonTextStyle_Error::UCkCommonTextStyle_Error()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.88f, 0.37f, 0.37f); // Red
}
