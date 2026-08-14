#include "CkWidgets/Styles/CkCommonTextStyle_Success.h"

UCkCommonTextStyle_Success::UCkCommonTextStyle_Success()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 12);
    Color = FLinearColor(0.38f, 0.80f, 0.44f); // Green
}
