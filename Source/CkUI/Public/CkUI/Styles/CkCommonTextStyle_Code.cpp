#include "CkCommonTextStyle_Code.h"

UCkCommonTextStyle_Code::UCkCommonTextStyle_Code()
{
    Font.Size = 11;
    Font.TypefaceFontName = FName("Mono"); // Monospace variant of your font
    Color = FLinearColor(0.75f, 0.80f, 0.85f); // Slightly bluish-gray
}
