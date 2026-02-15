#include "CkWatermark_Types.h"

// --------------------------------------------------------------------------------------------------------------------

FLinearColor
    FCk_Watermark_ColorBands_HigherIsBetter::
    GetColorForValue(float InValue) const
{
    if (InValue >= VeryGood_Threshold) { return VeryGood_Color; }
    if (InValue >= Good_Threshold)     { return Good_Color; }
    if (InValue >= Okay_Threshold)     { return Okay_Color; }
    if (InValue >= Bad_Threshold)      { return Bad_Color; }
    return VeryBad_Color;
}

// --------------------------------------------------------------------------------------------------------------------

FLinearColor
    FCk_Watermark_ColorBands_LowerIsBetter::
    GetColorForValue(float InValue) const
{
    if (InValue <= VeryGood_Threshold) { return VeryGood_Color; }
    if (InValue <= Good_Threshold)     { return Good_Color; }
    if (InValue <= Okay_Threshold)     { return Okay_Color; }
    if (InValue <= Bad_Threshold)      { return Bad_Color; }
    return VeryBad_Color;
}

// --------------------------------------------------------------------------------------------------------------------
