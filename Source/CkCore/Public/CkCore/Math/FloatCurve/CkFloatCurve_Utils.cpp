#include "CkFloatCurve_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatCurve_UE::
    Get_RuntimeFloatCurve_ValueAtTime(
        const FRuntimeFloatCurve& InFloatCurve,
        const FCk_Time& InTime)
    -> float
{
    const auto& richCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(richCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    const auto& seconds = InTime.Get_Seconds();
    return richCurve->Eval(seconds);
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_RuntimeFloatCurve_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    const auto& richCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(richCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    auto outMinTime = 0.0f;
    auto outMaxTime = 0.0f;
    richCurve->GetTimeRange(outMinTime, outMaxTime);

    return FCk_FloatRange{ outMinTime, outMaxTime };
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_RuntimeFloatCurve_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    const auto& richCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(richCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    auto outMinValue = 0.0f;
    auto outMaxValue = 0.0f;
    richCurve->GetValueRange(outMinValue, outMaxValue);

    return FCk_FloatRange{ outMinValue, outMaxValue };
}

// --------------------------------------------------------------------------------------------------------------------