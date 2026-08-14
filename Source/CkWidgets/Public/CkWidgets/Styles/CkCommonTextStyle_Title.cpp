#include "CkWidgets/Styles/CkCommonTextStyle_Title.h"

UCkCommonTextStyle_Title::UCkCommonTextStyle_Title()
{
    Font = FCoreStyle::GetDefaultFontStyle("Bold", 22);
    Color = FLinearColor(0.92f, 0.92f, 0.92f); // Near-white, one step above Header
}
