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
    const auto& RichCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(RichCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    const auto& Seconds = InTime.Get_Seconds();
    return RichCurve->Eval(Seconds);
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_RuntimeFloatCurve_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    const auto& RichCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(RichCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    auto OutMinTime = 0.0f;
    auto OutMaxTime = 0.0f;
    RichCurve->GetTimeRange(OutMinTime, OutMaxTime);

    return FCk_FloatRange{ OutMinTime, OutMaxTime };
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_RuntimeFloatCurve_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    const auto& RichCurve = InFloatCurve.GetRichCurveConst();

    CK_ENSURE_IF_NOT(ck::IsValid(RichCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid RichCurve for RuntimeFloatCurve"))
    { return {}; }

    auto OutMinValue = 0.0f;
    auto OutMaxValue = 0.0f;
    RichCurve->GetValueRange(OutMinValue, OutMaxValue);

    return FCk_FloatRange{ OutMinValue, OutMaxValue };
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_IsValid_RuntimeFloatCurve(
        const FRuntimeFloatCurve& InFloatCurve)
    -> bool
{
    const auto& RichCurve = InFloatCurve.GetRichCurveConst();

    return ck::IsValid(RichCurve, ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
