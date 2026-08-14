#include "CkWidgets/Styles/CkCommonTextStyle_Warning.h"

UCkCommonTextStyle_Warning::UCkCommonTextStyle_Warning()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.95f, 0.72f, 0.25f); // Amber
}
