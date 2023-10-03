#include "CkNumericLimits_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatNumericLimits_UE::
    Get_IsNumericLimitMax(
        float InValue)
    -> bool
{
    return FMath::IsNearlyEqual(InValue, std::numeric_limits<float>().max());
}

auto
    UCk_Utils_FloatNumericLimits_UE::
    Get_IsNumericLimitMin(
        float InValue)
    -> bool
{
    return FMath::IsNearlyEqual(InValue, std::numeric_limits<float>().lowest());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DoubleNumericLimits_UE::
    Get_IsNumericLimitMax(
        double InValue)
    -> bool
{
    return FMath::IsNearlyEqual(InValue, std::numeric_limits<double>().max());
}

auto
    UCk_Utils_DoubleNumericLimits_UE::
    Get_IsNumericLimitMin(
        double InValue)
    -> bool
{
    return FMath::IsNearlyEqual(InValue, std::numeric_limits<double>().lowest());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Int32NumericLimits_UE::
    Get_IsNumericLimitMax(
        int32 InValue)
    -> bool
{
    return InValue == std::numeric_limits<int32>().max();
}

auto
    UCk_Utils_Int32NumericLimits_UE::
    Get_IsNumericLimitMin(
        int32 InValue)
    -> bool
{
    return InValue == std::numeric_limits<int32>().lowest();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Int64NumericLimits_UE::
    Get_IsNumericLimitMax(
        int64 InValue)
    -> bool
{
    return InValue == std::numeric_limits<int64>().max();
}

auto
    UCk_Utils_Int64NumericLimits_UE::
    Get_IsNumericLimitMin(
        int64 InValue)
    -> bool
{
    return InValue == std::numeric_limits<int64>().max();
}

// --------------------------------------------------------------------------------------------------------------------