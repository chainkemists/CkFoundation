#include "CkCurve_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_Curve_UE::
    Get_ValueAtTime(
        const FRichCurve& InRichCurve,
        const FCk_Time& InTime)
    -> float
{
    const auto& Seconds = InTime.Get_Seconds();
    return InRichCurve.Eval(Seconds);
}

auto
    UCk_Utils_Curve_UE::
    Get_TimeRange(
        const FRichCurve& InRichCurve)
    -> FCk_FloatRange
{
    auto OutMinTime = 0.0f;
    auto OutMaxTime = 0.0f;
    InRichCurve.GetTimeRange(OutMinTime, OutMaxTime);

    return FCk_FloatRange{ OutMinTime, OutMaxTime };
}

auto
    UCk_Utils_Curve_UE::
    Get_ValueRange(
        const FRichCurve& InRichCurve)
    -> FCk_FloatRange
{
    auto OutMinValue = 0.0f;
    auto OutMaxValue = 0.0f;
    InRichCurve.GetValueRange(OutMinValue, OutMaxValue);

    return FCk_FloatRange{ OutMinValue, OutMaxValue };
}

// --------------------------------------------------------------------------------------------------------------------
