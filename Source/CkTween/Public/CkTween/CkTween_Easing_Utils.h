#pragma once

#include "CkTween_Fragment_Data.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

// --------------------------------------------------------------------------------------------------------------------

class CKTWEEN_API UCk_Utils_TweenEasing_UE
{
public:
    // Apply easing function to normalized time (0.0 to 1.0)
    static auto Apply(ECk_TweenEasing InEasing, FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Interpolate between two values using easing
    static auto Interpolate(const FCk_TweenValue& InStart, const FCk_TweenValue& InEnd, FCk_FloatRange_0to1 InT, ECk_TweenEasing InEasing) -> FCk_TweenValue;

private:
    // Individual easing functions
    static auto Linear(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // ReSharper disable CppInconsistentNaming
    // Sine
    static auto InSine(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutSine(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutSine(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Quad
    static auto InQuad(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutQuad(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutQuad(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Cubic
    static auto InCubic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutCubic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutCubic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Quart
    static auto InQuart(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutQuart(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutQuart(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Quint
    static auto InQuint(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutQuint(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutQuint(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Expo
    static auto InExpo(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutExpo(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutExpo(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Circ
    static auto InCirc(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutCirc(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutCirc(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Back
    static auto InBack(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutBack(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutBack(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Elastic
    static auto InElastic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutElastic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutElastic(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Bounce
    static auto InBounce(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto OutBounce(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;
    static auto InOutBounce(FCk_FloatRange_0to1 InT) -> FCk_FloatRange_0to1;

    // Type-specific interpolation helpers
    static auto InterpolateFloat(float InStart, float InEnd, FCk_FloatRange_0to1 InT) -> float;
    static auto InterpolateVector(const FVector& InStart, const FVector& InEnd, FCk_FloatRange_0to1 InT) -> FVector;
    static auto InterpolateRotator(const FRotator& InStart, const FRotator& InEnd, FCk_FloatRange_0to1 InT) -> FRotator;
    static auto InterpolateLinearColor(const FLinearColor& InStart, const FLinearColor& InEnd, FCk_FloatRange_0to1 InT) -> FLinearColor;
};

// --------------------------------------------------------------------------------------------------------------------
