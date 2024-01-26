#include "CkCurveTable_Utils.h"

#include "CkCore/Curve/CkCurve_Utils.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "Engine/CurveTable.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CurveTable_UE::
    Get_ValueAtTime(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        const FCk_Time& InTime,
        ECk_RowFoundOrNot& OutResult)
    -> float
{
    OutResult = ECk_RowFoundOrNot::RowNotFound;
    const auto& Result = Get_ValueAtTime(InCurveTableRowHandle, InTime);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    OutResult = ECk_RowFoundOrNot::RowFound;
    return *Result;
}

auto
    UCk_Utils_CurveTable_UE::
    Get_TimeRange(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        ECk_RowFoundOrNot& OutResult)
    -> FCk_FloatRange
{
    OutResult = ECk_RowFoundOrNot::RowNotFound;
    const auto& Result = Get_TimeRange(InCurveTableRowHandle);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    OutResult = ECk_RowFoundOrNot::RowFound;
    return *Result;
}

auto
    UCk_Utils_CurveTable_UE::
    Get_ValueRange(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        ECk_RowFoundOrNot& OutResult)
    -> FCk_FloatRange
{
    OutResult = ECk_RowFoundOrNot::RowNotFound;
    const auto& Result = Get_ValueRange(InCurveTableRowHandle);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    OutResult = ECk_RowFoundOrNot::RowFound;
    return *Result;
}

auto
    UCk_Utils_CurveTable_UE::
    Get_IsValid_RowHandle(
        const FCurveTableRowHandle& InCurveTableRowHandle)
    -> bool
{
    return ck::IsValid(InCurveTableRowHandle);
}

auto
    UCk_Utils_CurveTable_UE::
    Get_ValueAtTime(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        const FCk_Time& InTime)
    -> TOptional<float>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCurveTableRowHandle),
        TEXT("Invalid Curve Table Row Handle when trying to get Value At Time"))
    { return {};}

    static FString ContextString;

    return UCk_Utils_Curve_UE::Get_ValueAtTime(*InCurveTableRowHandle.GetRichCurve(ContextString), InTime);
}

auto
    UCk_Utils_CurveTable_UE::
    Get_TimeRange(
        const FCurveTableRowHandle& InCurveTableRowHandle)
    -> TOptional<FCk_FloatRange>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCurveTableRowHandle),
        TEXT("Invalid Curve Table Row Handle when trying to get Time Range"))
    { return {};}

    static FString ContextString;

    return UCk_Utils_Curve_UE::Get_TimeRange(*InCurveTableRowHandle.GetRichCurve(ContextString));
}

auto
    UCk_Utils_CurveTable_UE::
    Get_ValueRange(
        const FCurveTableRowHandle& InCurveTableRowHandle)
    -> TOptional<FCk_FloatRange>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCurveTableRowHandle),
        TEXT("Invalid Curve Table Row Handle when trying to get Value Range"))
    { return {};}

    static FString ContextString;

    return UCk_Utils_Curve_UE::Get_ValueRange(*InCurveTableRowHandle.GetRichCurve(ContextString));
}

// --------------------------------------------------------------------------------------------------------------------
