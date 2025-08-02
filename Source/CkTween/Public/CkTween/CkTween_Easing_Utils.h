#pragma once

#include "CkTween_Fragment_Data.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

// --------------------------------------------------------------------------------------------------------------------

class CKTWEEN_API UCk_Utils_TweenEasing_UE
{
public:
    // Apply easing function to normalized time (0.0 to 1.0)
    static auto Apply(ECk_TweenEasing InEasing, FCk_FloatRange_0to1 InT) -> float;

    // Interpolate between two values using easing
    static auto Interpolate(const FCk_TweenValue& InStart, const FCk_TweenValue& InEnd, FCk_FloatRange_0to1 InT, ECk_TweenEasing InEasing) -> FCk_TweenValue;

private:
    // Individual easing functions
    static auto Linear(FCk_FloatRange_0to1 InT) -> float;

    // ReSharper disable CppInconsistentNaming

    // Sine
    static auto InSine(FCk_FloatRange_0to1 InT) -> float;
    static auto OutSine(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutSine(FCk_FloatRange_0to1 InT) -> float;

    // Quad
    static auto InQuad(FCk_FloatRange_0to1 InT) -> float;
    static auto OutQuad(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutQuad(FCk_FloatRange_0to1 InT) -> float;

    // Cubic
    static auto InCubic(FCk_FloatRange_0to1 InT) -> float;
    static auto OutCubic(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutCubic(FCk_FloatRange_0to1 InT) -> float;

    // Quart
    static auto InQuart(FCk_FloatRange_0to1 InT) -> float;
    static auto OutQuart(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutQuart(FCk_FloatRange_0to1 InT) -> float;

    // Quint
    static auto InQuint(FCk_FloatRange_0to1 InT) -> float;
    static auto OutQuint(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutQuint(FCk_FloatRange_0to1 InT) -> float;

    // Expo
    static auto InExpo(FCk_FloatRange_0to1 InT) -> float;
    static auto OutExpo(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutExpo(FCk_FloatRange_0to1 InT) -> float;

    // Circ
    static auto InCirc(FCk_FloatRange_0to1 InT) -> float;
    static auto OutCirc(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutCirc(FCk_FloatRange_0to1 InT) -> float;

    // Back
    static auto InBack(FCk_FloatRange_0to1 InT) -> float;
    static auto OutBack(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutBack(FCk_FloatRange_0to1 InT) -> float;

    // Elastic
    static auto InElastic(FCk_FloatRange_0to1 InT) -> float;
    static auto OutElastic(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutElastic(FCk_FloatRange_0to1 InT) -> float;

    // Bounce
    static auto InBounce(FCk_FloatRange_0to1 InT) -> float;
    static auto OutBounce(FCk_FloatRange_0to1 InT) -> float;
    static auto InOutBounce(FCk_FloatRange_0to1 InT) -> float;

    // Type-specific interpolation helpers
    static auto InterpolateFloat(float InStart, float InEnd, float InT) -> float;
    static auto InterpolateVector(const FVector& InStart, const FVector& InEnd, float InT) -> FVector;
    static auto InterpolateRotator(const FRotator& InStart, const FRotator& InEnd, float InT) -> FRotator;
    static auto InterpolateLinearColor(const FLinearColor& InStart, const FLinearColor& InEnd, float InT) -> FLinearColor;

    // ReSharper restore CppInconsistentNaming
};

// --------------------------------------------------------------------------------------------------------------------
