#include "CkArithmetic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Arithmetic_UE::
    Increment_WithWrap(
        int32& InToIncrement,
        const FCk_IntRange& InRange)
    -> int32
{
    return Offset_WithWrap(InToIncrement, 1, InRange);
}

auto
    UCk_Utils_Arithmetic_UE::
    Decrement_WithWrap(
        int32&              InToDecrement,
        const FCk_IntRange& InRange)
    -> int32
{
    return Offset_WithWrap(InToDecrement, -1, InRange);
}

auto
    UCk_Utils_Arithmetic_UE::
    Offset_WithWrap(
        int32&              InToJump,
        int32               InOffset,
        const FCk_IntRange& InRange)
    -> int32
{
    InToJump = ((InToJump + InOffset)%InRange.Get_Max() + InRange.Get_Max()) % InRange.Get_Max();

    return InToJump;
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
    Get_AngleFromDotProduct_Degrees(
        float InDotProduct)
    -> float
{
    return FMath::RadiansToDegrees(Get_AngleFromDotProduct_Radians(InDotProduct));
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_AngleFromDotProduct_Radians(
        float InDotProduct)
    -> float
{
    CK_ENSURE_IF_NOT(UCk_Utils_FloatRange_UE::Get_IsWithinRange(InDotProduct, FCk_FloatRange{ -1.0f, 1.0f }, ECk_Inclusiveness::Inclusive),
        TEXT("Trying to calculate the Angle from a Dot Product result [{}] that is NOT within the range [-1.0, 1.0]."
             "Clamping the value within the working range to avoid domain error in Non-Shipping build"), InDotProduct)
    {
        InDotProduct = FMath::Clamp(InDotProduct, -1.0f, 1.0f);
    }

    return FMath::Acos(InDotProduct);
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
    return Offset_WithWrap(InToIncrement, 1, InRange);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_Decrement_WithWrap(
        int32               InToDecrement,
        const FCk_IntRange& InRange)
    -> int32
{
    return Offset_WithWrap(InToDecrement, -1, InRange);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_Offset_WithWrap(
        int32               InToDecrement,
        int32               InOffset,
        const FCk_IntRange& InRange)
    -> int32
{
    return Offset_WithWrap(InToDecrement, InOffset, InRange);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_IsNearlyEqual(
        uint8 A,
        uint8 B)
    -> bool
{
    return A == B;
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_IsNearlyEqual(
        int32 A,
        int32 B)
    -> bool
{
    return A == B;
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_IsNearlyEqual(
        float A,
        float B)
    -> bool
{
    return FMath::IsNearlyEqual(A, B);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_IsNearlyEqual(
        double A,
        double B)
    -> bool
{
    return FMath::IsNearlyEqual(A, B);
}

auto
    UCk_Utils_Arithmetic_UE::
    Get_IsNearlyEqual(
        const FVector& A,
        const FVector& B)
    -> bool
{
    return Get_IsNearlyEqual(A.X, B.X) &&
        Get_IsNearlyEqual(A.Y, B.Y) &&
        Get_IsNearlyEqual(A.Z, B.Z);
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
