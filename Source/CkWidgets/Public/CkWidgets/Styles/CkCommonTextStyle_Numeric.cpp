#include "CkWidgets/Styles/CkCommonTextStyle_Numeric.h"

UCkCommonTextStyle_Numeric::UCkCommonTextStyle_Numeric()
{
    // Monospaced digits keep columns of prices/quantities aligned
    Font = FCoreStyle::GetDefaultFontStyle("Mono", 12);
    Color = FLinearColor(0.85f, 0.85f, 0.85f); // Light gray
}
