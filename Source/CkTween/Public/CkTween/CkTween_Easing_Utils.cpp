#include "CkTween_Easing_Utils.h"

#include "CkCore/Math/ValueRange/CkValueRange_Utils.h"

#include <cmath>

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::Apply(ECk_TweenEasing InEasing, FCk_FloatRange_0to1 InT) -> float
{
    switch (InEasing)
    {
	    case ECk_TweenEasing::Linear: return Linear(InT);

	    case ECk_TweenEasing::InSine: return InSine(InT);
	    case ECk_TweenEasing::OutSine: return OutSine(InT);
	    case ECk_TweenEasing::InOutSine: return InOutSine(InT);

	    case ECk_TweenEasing::InQuad: return InQuad(InT);
	    case ECk_TweenEasing::OutQuad: return OutQuad(InT);
	    case ECk_TweenEasing::InOutQuad: return InOutQuad(InT);

	    case ECk_TweenEasing::InCubic: return InCubic(InT);
	    case ECk_TweenEasing::OutCubic: return OutCubic(InT);
	    case ECk_TweenEasing::InOutCubic: return InOutCubic(InT);

	    case ECk_TweenEasing::InQuart: return InQuart(InT);
	    case ECk_TweenEasing::OutQuart: return OutQuart(InT);
	    case ECk_TweenEasing::InOutQuart: return InOutQuart(InT);

	    case ECk_TweenEasing::InQuint: return InQuint(InT);
	    case ECk_TweenEasing::OutQuint: return OutQuint(InT);
	    case ECk_TweenEasing::InOutQuint: return InOutQuint(InT);

	    case ECk_TweenEasing::InExpo: return InExpo(InT);
	    case ECk_TweenEasing::OutExpo: return OutExpo(InT);
	    case ECk_TweenEasing::InOutExpo: return InOutExpo(InT);

	    case ECk_TweenEasing::InCirc: return InCirc(InT);
	    case ECk_TweenEasing::OutCirc: return OutCirc(InT);
	    case ECk_TweenEasing::InOutCirc: return InOutCirc(InT);

	    case ECk_TweenEasing::InBack: return InBack(InT);
	    case ECk_TweenEasing::OutBack: return OutBack(InT);
	    case ECk_TweenEasing::InOutBack: return InOutBack(InT);

	    case ECk_TweenEasing::InElastic: return InElastic(InT);
	    case ECk_TweenEasing::OutElastic: return OutElastic(InT);
	    case ECk_TweenEasing::InOutElastic: return InOutElastic(InT);

	    case ECk_TweenEasing::InBounce: return InBounce(InT);
	    case ECk_TweenEasing::OutBounce: return OutBounce(InT);
	    case ECk_TweenEasing::InOutBounce: return InOutBounce(InT);

	    default: return Linear(InT);
    }
}

auto UCk_Utils_TweenEasing_UE::Interpolate(const FCk_TweenValue& InStart, const FCk_TweenValue& InEnd, FCk_FloatRange_0to1 InT, ECk_TweenEasing InEasing) -> FCk_TweenValue
{
    const auto EasedT = Apply(InEasing, InT);

    return std::visit([&]<typename T0>(const T0& Start) -> FCk_TweenValue
    {
        using T = std::decay_t<T0>;
        const auto& End = InEnd.Get<T>();

        if constexpr (std::is_same_v<T, float>)
        {
            return InterpolateFloat(Start, End, EasedT);
        }
        else if constexpr (std::is_same_v<T, FVector>)
        {
            return InterpolateVector(Start, End, EasedT);
        }
        else if constexpr (std::is_same_v<T, FRotator>)
        {
            return InterpolateRotator(Start, End, EasedT);
        }
        else if constexpr (std::is_same_v<T, FLinearColor>)
        {
            return InterpolateLinearColor(Start, End, EasedT);
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unsupported tween type");
            return FCk_TweenValue{};
        }
    }, InStart.GetVariant());
}

// --------------------------------------------------------------------------------------------------------------------
// Linear
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::Linear(FCk_FloatRange_0to1 InT) -> float
{
    return InT.Get_Value();
}

// --------------------------------------------------------------------------------------------------------------------
// Sine
// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable CppInconsistentNaming

auto UCk_Utils_TweenEasing_UE::InSine(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - FMath::Cos(T * PI / 2.0f);
}

auto UCk_Utils_TweenEasing_UE::OutSine(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return FMath::Sin(T * PI / 2.0f);
}

auto UCk_Utils_TweenEasing_UE::InOutSine(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return -(FMath::Cos(PI * T) - 1.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Quad
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InQuad(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T * T;
}

auto UCk_Utils_TweenEasing_UE::OutQuad(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - (1.0f - T) * (1.0f - T);
}

auto UCk_Utils_TweenEasing_UE::InOutQuad(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f ? 2.0f * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 2.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Cubic
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InCubic(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T * T * T;
}

auto UCk_Utils_TweenEasing_UE::OutCubic(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - FMath::Pow(1.0f - T, 3.0f);
}

auto UCk_Utils_TweenEasing_UE::InOutCubic(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f ? 4.0f * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 3.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Quart
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InQuart(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T * T * T * T;
}

auto UCk_Utils_TweenEasing_UE::OutQuart(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - FMath::Pow(1.0f - T, 4.0f);
}

auto UCk_Utils_TweenEasing_UE::InOutQuart(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f ? 8.0f * T * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 4.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Quint
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InQuint(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T * T * T * T * T;
}

auto UCk_Utils_TweenEasing_UE::OutQuint(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - FMath::Pow(1.0f - T, 5.0f);
}

auto UCk_Utils_TweenEasing_UE::InOutQuint(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f ? 16.0f * T * T * T * T * T : 1.0f - FMath::Pow(-2.0f * T + 2.0f, 5.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Expo
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InExpo(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T == 0.0f ? 0.0f : FMath::Pow(2.0f, 10.0f * (T - 1.0f));
}

auto UCk_Utils_TweenEasing_UE::OutExpo(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T == 1.0f ? 1.0f : 1.0f - FMath::Pow(2.0f, -10.0f * T);
}

auto UCk_Utils_TweenEasing_UE::InOutExpo(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();

    if (T == 0.0f) return 0.0f;
    if (T == 1.0f) return 1.0f;

    return T < 0.5f
        ? FMath::Pow(2.0f, 20.0f * T - 10.0f) / 2.0f
        : (2.0f - FMath::Pow(2.0f, -20.0f * T + 10.0f)) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Circ
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InCirc(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return 1.0f - FMath::Sqrt(1.0f - FMath::Pow(T, 2.0f));
}

auto UCk_Utils_TweenEasing_UE::OutCirc(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return FMath::Sqrt(1.0f - FMath::Pow(T - 1.0f, 2.0f));
}

auto UCk_Utils_TweenEasing_UE::InOutCirc(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f
        ? (1.0f - FMath::Sqrt(1.0f - FMath::Pow(2.0f * T, 2.0f))) / 2.0f
        : (FMath::Sqrt(1.0f - FMath::Pow(-2.0f * T + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Back
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InBack(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C1 = 1.70158f;
    constexpr float C3 = C1 + 1.0f;

    const auto T = InT.Get_Value();
    return C3 * T * T * T - C1 * T * T;
}

auto UCk_Utils_TweenEasing_UE::OutBack(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C1 = 1.70158f;
    constexpr float C3 = C1 + 1.0f;

    const auto T = InT.Get_Value();
    return 1.0f + C3 * FMath::Pow(T - 1.0f, 3.0f) + C1 * FMath::Pow(T - 1.0f, 2.0f);
}

auto UCk_Utils_TweenEasing_UE::InOutBack(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C1 = 1.70158f;
    constexpr float C2 = C1 * 1.525f;

    const auto T = InT.Get_Value();
    return T < 0.5f
        ? (FMath::Pow(2.0f * T, 2.0f) * ((C2 + 1.0f) * 2.0f * T - C2)) / 2.0f
        : (FMath::Pow(2.0f * T - 2.0f, 2.0f) * ((C2 + 1.0f) * (T * 2.0f - 2.0f) + C2) + 2.0f) / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Elastic
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InElastic(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C4 = (2.0f * PI) / 3.0f;

    const auto T = InT.Get_Value();
    if (T == 0.0f) return 0.0f;
    if (T == 1.0f) return 1.0f;

    return -FMath::Pow(2.0f, 10.0f * (T - 1.0f)) * FMath::Sin((T - 1.0f) * C4 - C4);
}

auto UCk_Utils_TweenEasing_UE::OutElastic(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C4 = (2.0f * PI) / 3.0f;

    const auto T = InT.Get_Value();
    if (T == 0.0f) return 0.0f;
    if (T == 1.0f) return 1.0f;

    return FMath::Pow(2.0f, -10.0f * T) * FMath::Sin(T * C4 - C4) + 1.0f;
}

auto UCk_Utils_TweenEasing_UE::InOutElastic(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float C5 = (2.0f * PI) / 4.5f;

    const auto T = InT.Get_Value();
    if (T == 0.0f) return 0.0f;
    if (T == 1.0f) return 1.0f;

    return T < 0.5f
        ? -(FMath::Pow(2.0f, 20.0f * T - 10.0f) * FMath::Sin((20.0f * T - 11.125f) * C5)) / 2.0f
        : (FMath::Pow(2.0f, -20.0f * T + 10.0f) * FMath::Sin((20.0f * T - 11.125f) * C5)) / 2.0f + 1.0f;
}

// --------------------------------------------------------------------------------------------------------------------
// Bounce
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InBounce(FCk_FloatRange_0to1 InT) -> float
{
    return 1.0f - OutBounce(UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(1.0f - InT.Get_Value()));
}

auto UCk_Utils_TweenEasing_UE::OutBounce(FCk_FloatRange_0to1 InT) -> float
{
    constexpr float N1 = 7.5625f;
    constexpr float D1 = 2.75f;

    auto T = InT.Get_Value();

    if (T < 1.0f / D1)
    {
        return N1 * T * T;
    }

    if (T < 2.0f / D1)
    {
        T -= 1.5f / D1;
        return N1 * T * T + 0.75f;
    }

    if (T < 2.5f / D1)
    {
        T -= 2.25f / D1;
        return N1 * T * T + 0.9375f;
    }

    T -= 2.625f / D1;
    return N1 * T * T + 0.984375f;
}

auto UCk_Utils_TweenEasing_UE::InOutBounce(FCk_FloatRange_0to1 InT) -> float
{
    const auto T = InT.Get_Value();
    return T < 0.5f
        ? (1.0f - OutBounce(UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(1.0f - 2.0f * T))) / 2.0f
        : (1.0f + OutBounce(UCk_Utils_FloatRange_0to1_UE::Make_FloatRange_0to1(2.0f * T - 1.0f))) / 2.0f;
}

// ReSharper restore CppInconsistentNaming

// --------------------------------------------------------------------------------------------------------------------
// Type-specific interpolation
// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_TweenEasing_UE::InterpolateFloat(float InStart, float InEnd, float InT) -> float
{
    return FMath::Lerp(InStart, InEnd, InT);
}

auto UCk_Utils_TweenEasing_UE::InterpolateVector(const FVector& InStart, const FVector& InEnd, float InT) -> FVector
{
    return FMath::Lerp(InStart, InEnd, InT);
}

auto UCk_Utils_TweenEasing_UE::InterpolateRotator(const FRotator& InStart, const FRotator& InEnd, float InT) -> FRotator
{
    return FMath::Lerp(InStart, InEnd, InT);
}

auto UCk_Utils_TweenEasing_UE::InterpolateLinearColor(const FLinearColor& InStart, const FLinearColor& InEnd, float InT) -> FLinearColor
{
    return FMath::Lerp(InStart, InEnd, InT);
}

// --------------------------------------------------------------------------------------------------------------------
