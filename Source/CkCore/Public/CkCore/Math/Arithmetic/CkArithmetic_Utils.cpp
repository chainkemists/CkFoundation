#include "CkArithmetic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Arithmetic_UE::
    Increment_WithWrap(
        int32& InToIncrement,
        const FCk_IntRange& InRange)
    -> int32
{
    ++InToIncrement;
    if (InRange.Get_IsWithinExclusive(InToIncrement))
    {
        InToIncrement = InRange.Get_Min();
    }

    return InToIncrement;
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_LerpFloatWithEasing(
        float InA,
        float InB,
        float InAlpha,
        float InPower,
        ECk_EasingMethod InEasingMethod)
    -> float
{
    switch (InEasingMethod)
    {
        case ECk_EasingMethod::EaseIn:
        {
            return FMath::InterpEaseIn(InA, InB, InAlpha, InPower);
        }
        case ECk_EasingMethod::EaseOut:
        {
            return FMath::InterpEaseOut(InA, InB, InAlpha, InPower);
        }
        case ECk_EasingMethod::EaseInAndOut:
        {
            return FMath::InterpEaseInOut(InA, InB, InAlpha, InPower);
        }
        default:
        {
            CK_INVALID_ENUM(InEasingMethod);
            return {};
        }
    }
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_RoundFloatToFloat(
        ECk_RoundingMethod InRoundingMethod,
        float InValue)
    -> float
{
    switch(InRoundingMethod)
    {
        case ECk_RoundingMethod::Ceiling:
        {
            return FMath::CeilToFloat(InValue);
        }
        case ECk_RoundingMethod::Floor:
        {
            return FMath::FloorToFloat(InValue);
        }
        case ECk_RoundingMethod::Closest:
        {
            return FMath::RoundToFloat(InValue);
        }
        default:
        {
            CK_INVALID_ENUM(InRoundingMethod);
            return InValue;
        }
    }
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_RoundFloatToInt(
        ECk_RoundingMethod InRoundingMethod,
        float InValue)
    -> int32
{
    return Get_RoundFloatToFloat(InRoundingMethod, InValue);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_Increment_WithWrap(
        int32               InToIncrement,
        const FCk_IntRange& InRange)
    -> int32
{
    Increment_WithWrap(InToIncrement, InRange);
    return InToIncrement;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T>
    auto
        Negate(
            const T& InValue)
        -> T
    {
        return InValue * T{-1};
    }
}


// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template auto Negate(const FVector2D&) -> FVector2D;
    template auto Negate(const FVector&) -> FVector;
    template auto Negate(const float&) -> float;
    template auto Negate(const double&) -> double;
    template auto Negate(const int&) -> int;
}
