#pragma once

#include "CkComparison_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

namespace ck_comparison
{
    template <typename T>
    auto
        Get_IsComparisonTrue(
            T InLHS,
            T InRHS,
            ECk_ComparisonOperators InComparison,
            T InTolerance)
        -> bool
    {
        switch(InComparison)
        {
            case ECk_ComparisonOperators::EqualTo:
                return std::abs(InLHS - InRHS) <= InTolerance;
            case ECk_ComparisonOperators::NotEqualTo:
                return NOT (std::abs(InLHS - InRHS) <= InTolerance);
            case ECk_ComparisonOperators::GreaterThan:
                return InLHS > InRHS;
            case ECk_ComparisonOperators::LessThan:
                return InLHS < InRHS;
            case ECk_ComparisonOperators::GreaterThanOrEqualTo:
                return InLHS >= InRHS;
            case ECk_ComparisonOperators::LessThanOrEqualTo:
                return InLHS <= InRHS;
            default:
                CK_INVALID_ENUM(InComparison);
                return false;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntComparison_UE::
    Get_IsComparisonTrue(
        int32 InLHS,
        const FCk_Comparison_Int& InComparison)
    -> bool
{
    return ck_comparison::Get_IsComparisonTrue(InLHS, InComparison.Get_RHS(), InComparison.Get_Operator(), 0);
}

auto
    UCk_Utils_IntComparison_UE::
    Get_IsInRange(
        int32 InNum,
        const FCk_Comparison_IntRange& InComparison)
    -> bool
{
    const auto Lhs = InComparison.Get_LHS();
    const auto Rhs = InComparison.Get_RHS();

    switch(const auto& Logic = InComparison.Get_Logic())
    {
    case ECk_Logic_And_Or::And:
        return ck_comparison::Get_IsComparisonTrue(Lhs, InNum, InComparison.Get_OperatorLHS(), 0) &&
               ck_comparison::Get_IsComparisonTrue(InNum, Rhs, InComparison.Get_OperatorRHS(), 0);
    case ECk_Logic_And_Or::Or:
        return ck_comparison::Get_IsComparisonTrue(Lhs, InNum, InComparison.Get_OperatorLHS(), 0) ||
               ck_comparison::Get_IsComparisonTrue(InNum, Rhs, InComparison.Get_OperatorRHS(), 0);
    default:
        CK_INVALID_ENUM(Logic);
        return false;
    }
}

auto
    UCk_Utils_FloatComparison_UE::
    Get_IsComparisonTrue(
        float InLHS,
        const FCk_Comparison_Float& InComparison)
    -> bool
{
    return ck_comparison::Get_IsComparisonTrue(InLHS, InComparison.Get_RHS(), InComparison.Get_Operator(), InComparison.Get_Tolerance());
}

auto
    UCk_Utils_FloatComparison_UE::
    Get_IsInRange(
        float InNum,
        const FCk_Comparison_FloatRange& InComparison)
    -> bool
{
    const auto Lhs = InComparison.Get_LHS();
    const auto Rhs = InComparison.Get_RHS();

    switch(const auto& Logic = InComparison.Get_Logic())
    {
    case ECk_Logic_And_Or::And:
        return ck_comparison::Get_IsComparisonTrue(Lhs, InNum, InComparison.Get_OperatorLHS(), InComparison.Get_Tolerance()) &&
               ck_comparison::Get_IsComparisonTrue(InNum, Rhs, InComparison.Get_OperatorRHS(), InComparison.Get_Tolerance());
    case ECk_Logic_And_Or::Or:
        return ck_comparison::Get_IsComparisonTrue(Lhs, InNum, InComparison.Get_OperatorLHS(), InComparison.Get_Tolerance()) ||
               ck_comparison::Get_IsComparisonTrue(InNum, Rhs, InComparison.Get_OperatorRHS(), InComparison.Get_Tolerance());
    default:
        CK_INVALID_ENUM(Logic);
        return false;
    }
}

auto
	UCk_Utils_FloatComparison_UE::
	Get_IsLessThanOrNearlyEqual(
		float A,
		float B,
		float ErrorTolerance)
	-> bool
{
	return A <= B || FMath::IsNearlyEqual(A, B, ErrorTolerance);
}

auto
	UCk_Utils_FloatComparison_UE::
	Get_IsGreaterThanOrNearlyEqual(
		float A,
		float B,
		float ErrorTolerance)
	-> bool
{
	return A >= B || FMath::IsNearlyEqual(A, B, ErrorTolerance);
}

// --------------------------------------------------------------------------------------------------------------------