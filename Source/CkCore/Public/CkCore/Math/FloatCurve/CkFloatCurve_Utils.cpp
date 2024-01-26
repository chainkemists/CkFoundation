#include "CkFloatCurve_Utils.h"

#include "CkCore/Curve/CkCurve_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatCurve_UE::
    Get_ValueAtTime(
        const UCurveFloat* InFloatCurve,
        const FCk_Time&    InTime)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid Float Curve when trying to get Value At Time"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_ValueAtTime(InFloatCurve->FloatCurve, InTime);
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_TimeRange(
        const UCurveFloat* InFloatCurve)
    -> FCk_FloatRange
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid Float Curve when trying to get Time Range"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_TimeRange(InFloatCurve->FloatCurve);
}

auto
    UCk_Utils_FloatCurve_UE::
    Get_ValueRange(
        const UCurveFloat* InFloatCurve)
    -> FCk_FloatRange
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid Float Curve when trying to get Value Range"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_ValueRange(InFloatCurve->FloatCurve);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RuntimeFloatCurve_UE::
    Get_ValueAtTime(
        const FRuntimeFloatCurve& InFloatCurve,
        const FCk_Time& InTime)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve),
        TEXT("Invalid Runtime Float Curve when trying to get Value At Time"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_ValueAtTime(*InFloatCurve.GetRichCurveConst(), InTime);
}

auto
    UCk_Utils_RuntimeFloatCurve_UE::
    Get_TimeRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve),
        TEXT("Invalid Runtime Float Curve when trying to get Time Range"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_TimeRange(*InFloatCurve.GetRichCurveConst());
}

auto
    UCk_Utils_RuntimeFloatCurve_UE::
    Get_ValueRange(
        const FRuntimeFloatCurve& InFloatCurve)
    -> FCk_FloatRange
{
    CK_ENSURE_IF_NOT(ck::IsValid(InFloatCurve),
        TEXT("Invalid Runtime Float Curve when trying to get Value Range"))
    { return {}; }

    return UCk_Utils_Curve_UE::Get_ValueRange(*InFloatCurve.GetRichCurveConst());
}

auto
    UCk_Utils_RuntimeFloatCurve_UE::
    Get_IsValid(
        const FRuntimeFloatCurve& InFloatCurve)
    -> bool
{
    const auto& RichCurve = InFloatCurve.GetRichCurveConst();

    return ck::IsValid(RichCurve, ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
