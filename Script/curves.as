// --------------------------------------------------------------------------------------------------------------------
// curves.as - the easings.net easing functions as script-literal UCurveFloat assets.
//
// Every easing from https://easings.net/# is exposed as a curve::CurveFloatEase<Name> asset,
// usable anywhere a UCurveFloat is expected, e.g. with CkTween / CurveFloat.GetFloatValue(t).
//
//   auto V = curve::CurveFloatEaseOutBack.GetFloatValue(Alpha);
//
// HOW THE CUBIC-BEZIER CURVES ARE BUILT
//   easings.net publishes a CSS cubic-bezier(x1,y1,x2,y2) approximation for the Sine, Quad,
//   Cubic, Quart, Quint, Expo, Circ and Back families (24 functions). A CSS cubic bezier has
//   fixed endpoints P0=(0,0) and P3=(1,1) and two control points P1=(x1,y1), P2=(x2,y2).
//   We reproduce it exactly with two weighted-tangent keys: at each endpoint the tangent slope
//   is the direction toward its control point and the tangent weight is the distance to it
//   which is precisely how UE's RCTWM_WeightedBoth eval reconstructs the bezier handle
//   (handle = (cos(atan(slope)), sin(atan(slope))) * weight). So these curves are bit-faithful
//   to the CSS originals, not approximations.
//
// ELASTIC AND BOUNCE
//   The Elastic (x3) and Bounce (x3) easings oscillate / overshoot and have NO cubic-bezier
//   form - easings.net implements them as JS math, not a bezier. We sample the reference math
//   into a dense set of cubic-auto keys instead. If you only wanted the cubic-bezier set,
//   delete the "SAMPLED EASINGS" section and its 6 assets.
//
// Values transcribed from https://easings.net/# (easeInSine / easeOutSine confirmed against the
// two examples in the original request).
// --------------------------------------------------------------------------------------------------------------------

namespace curve
{
    // Detection threshold AND nudge magnitude for a vertical tangent handle (dx == 0). Only the
    // Circ end-variants hit this; every other control point has a non-zero dx.
    const float k_VerticalEpsilon = 1.0e-6;

    // ----------------------------------------------------------------------------------------------------------------
    // CUBIC-BEZIER PLUMBING
    // ----------------------------------------------------------------------------------------------------------------

    // Adds one weighted-tangent key whose handle points exactly at ControlPoint.
    mixin void AddCurveKeyWithCoordinateAndControlPoint(UCurveFloat CurveFloat, const FVector2D& Coordinate, const FVector2D& ControlPoint)
    {
        auto       DX = ControlPoint.X - Coordinate.X;
        const auto DY = ControlPoint.Y - Coordinate.Y;

        // Vertical handle -> infinite slope. Nudge DX to a tiny signed value so the slope stays
        // finite. The sign points the handle the right way: a start key (x < 0.5) leans right (+dx),
        // an end key leans left (-dx). Without the sign fix the arrive-side handle (easeInCirc)
        // would mirror to the wrong side.
        if (Math::Abs(DX) < k_VerticalEpsilon)
        {
            if (Coordinate.X < 0.5) { DX = k_VerticalEpsilon; }
            else                    { DX = -k_VerticalEpsilon; }
        }

        const auto Slope    = DY / DX;
        const auto Distance = Math::Sqrt(Math::Square(DX) + Math::Square(DY));

        const auto BrokenTangent = false;
        CurveFloat.AddCurveKeyWeightedBothTangent(Coordinate.X, Coordinate.Y, BrokenTangent, Slope, Slope, Distance, Distance);
    }

    // Converts a CSS cubic-bezier(x1,y1,x2,y2) into the two endpoint keys of the curve.
    mixin void CubicBezier(UCurveFloat CurveFloat, const float X1, const float Y1, const float X2, const float Y2)
    {
        CurveFloat.AddCurveKeyWithCoordinateAndControlPoint(FVector2D(0, 0), FVector2D(X1, Y1));
        CurveFloat.AddCurveKeyWithCoordinateAndControlPoint(FVector2D(1, 1), FVector2D(X2, Y2));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // SINE
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInSine
    asset CurveFloatEaseInSine of UCurveFloat
    {
        CubicBezier(0.12, 0, 0.39, 0);
    }

    // https://easings.net/#easeOutSine
    asset CurveFloatEaseOutSine of UCurveFloat
    {
        CubicBezier(0.61, 1, 0.88, 1);
    }

    // https://easings.net/#easeInOutSine
    asset CurveFloatEaseInOutSine of UCurveFloat
    {
        CubicBezier(0.37, 0, 0.63, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // QUAD
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInQuad
    asset CurveFloatEaseInQuad of UCurveFloat
    {
        CubicBezier(0.11, 0, 0.5, 0);
    }

    // https://easings.net/#easeOutQuad
    asset CurveFloatEaseOutQuad of UCurveFloat
    {
        CubicBezier(0.5, 1, 0.89, 1);
    }

    // https://easings.net/#easeInOutQuad
    asset CurveFloatEaseInOutQuad of UCurveFloat
    {
        CubicBezier(0.45, 0, 0.55, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // CUBIC
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInCubic
    asset CurveFloatEaseInCubic of UCurveFloat
    {
        CubicBezier(0.32, 0, 0.67, 0);
    }

    // https://easings.net/#easeOutCubic
    asset CurveFloatEaseOutCubic of UCurveFloat
    {
        CubicBezier(0.33, 1, 0.68, 1);
    }

    // https://easings.net/#easeInOutCubic
    asset CurveFloatEaseInOutCubic of UCurveFloat
    {
        CubicBezier(0.65, 0, 0.35, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // QUART
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInQuart
    asset CurveFloatEaseInQuart of UCurveFloat
    {
        CubicBezier(0.5, 0, 0.75, 0);
    }

    // https://easings.net/#easeOutQuart
    asset CurveFloatEaseOutQuart of UCurveFloat
    {
        CubicBezier(0.25, 1, 0.5, 1);
    }

    // https://easings.net/#easeInOutQuart
    asset CurveFloatEaseInOutQuart of UCurveFloat
    {
        CubicBezier(0.76, 0, 0.24, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // QUINT
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInQuint
    asset CurveFloatEaseInQuint of UCurveFloat
    {
        CubicBezier(0.64, 0, 0.78, 0);
    }

    // https://easings.net/#easeOutQuint
    asset CurveFloatEaseOutQuint of UCurveFloat
    {
        CubicBezier(0.22, 1, 0.36, 1);
    }

    // https://easings.net/#easeInOutQuint
    asset CurveFloatEaseInOutQuint of UCurveFloat
    {
        CubicBezier(0.83, 0, 0.17, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // EXPO
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInExpo
    asset CurveFloatEaseInExpo of UCurveFloat
    {
        CubicBezier(0.7, 0, 0.84, 0);
    }

    // https://easings.net/#easeOutExpo
    asset CurveFloatEaseOutExpo of UCurveFloat
    {
        CubicBezier(0.16, 1, 0.3, 1);
    }

    // https://easings.net/#easeInOutExpo
    asset CurveFloatEaseInOutExpo of UCurveFloat
    {
        CubicBezier(0.87, 0, 0.13, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // CIRC  (the In/Out variants have a vertical handle - see AddCurveKeyWithCoordinateAndControlPoint)
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInCirc
    asset CurveFloatEaseInCirc of UCurveFloat
    {
        CubicBezier(0.55, 0, 1, 0.45);
    }

    // https://easings.net/#easeOutCirc
    asset CurveFloatEaseOutCirc of UCurveFloat
    {
        CubicBezier(0, 0.55, 0.45, 1);
    }

    // https://easings.net/#easeInOutCirc
    asset CurveFloatEaseInOutCirc of UCurveFloat
    {
        CubicBezier(0.85, 0, 0.15, 1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // BACK  (overshoots past 0 / 1 - expected)
    // ----------------------------------------------------------------------------------------------------------------

    // https://easings.net/#easeInBack
    asset CurveFloatEaseInBack of UCurveFloat
    {
        CubicBezier(0.36, 0, 0.66, -0.56);
    }

    // https://easings.net/#easeOutBack
    asset CurveFloatEaseOutBack of UCurveFloat
    {
        CubicBezier(0.34, 1.56, 0.64, 1);
    }

    // https://easings.net/#easeInOutBack
    asset CurveFloatEaseInOutBack of UCurveFloat
    {
        CubicBezier(0.68, -0.6, 0.32, 1.6);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // SAMPLED EASINGS - Elastic & Bounce (no cubic-bezier form; sampled from the reference math)
    // ----------------------------------------------------------------------------------------------------------------

    enum ECk_Easing_Sampled
    {
        InElastic,
        OutElastic,
        InOutElastic,
        InBounce,
        OutBounce,
        InOutBounce
    }

    // https://easings.net/#easeOutBounce - the primitive the other two Bounce variants build on.
    float EaseOutBounce(float InX)
    {
        const auto N1 = 7.5625;
        const auto D1 = 2.75;

        auto X = InX;
        if (X < 1.0 / D1)
        {
            return N1 * X * X;
        }
        else if (X < 2.0 / D1)
        {
            X = X - 1.5 / D1;
            return N1 * X * X + 0.75;
        }
        else if (X < 2.5 / D1)
        {
            X = X - 2.25 / D1;
            return N1 * X * X + 0.9375;
        }

        X = X - 2.625 / D1;
        return N1 * X * X + 0.984375;
    }

    float EaseInBounce(float X)
    {
        return 1.0 - EaseOutBounce(1.0 - X);
    }

    float EaseInOutBounce(float X)
    {
        if (X < 0.5)
        {
            return (1.0 - EaseOutBounce(1.0 - 2.0 * X)) / 2.0;
        }
        return (1.0 + EaseOutBounce(2.0 * X - 1.0)) / 2.0;
    }

    float EaseInElastic(float X)
    {
        if (X <= 0.0) { return 0.0; }
        if (X >= 1.0) { return 1.0; }

        const auto C4 = (2.0 * Math::PI) / 3.0;
        return -Math::Pow(2.0, 10.0 * X - 10.0) * Math::Sin((X * 10.0 - 10.75) * C4);
    }

    float EaseOutElastic(float X)
    {
        if (X <= 0.0) { return 0.0; }
        if (X >= 1.0) { return 1.0; }

        const auto C4 = (2.0 * Math::PI) / 3.0;
        return Math::Pow(2.0, -10.0 * X) * Math::Sin((X * 10.0 - 0.75) * C4) + 1.0;
    }

    float EaseInOutElastic(float X)
    {
        if (X <= 0.0) { return 0.0; }
        if (X >= 1.0) { return 1.0; }

        const auto C5 = (2.0 * Math::PI) / 4.5;
        if (X < 0.5)
        {
            return -(Math::Pow(2.0, 20.0 * X - 10.0) * Math::Sin((20.0 * X - 11.125) * C5)) / 2.0;
        }
        return (Math::Pow(2.0, -20.0 * X + 10.0) * Math::Sin((20.0 * X - 11.125) * C5)) / 2.0 + 1.0;
    }

    float EvalSampledEasing(ECk_Easing_Sampled InEasing, float X)
    {
        switch (InEasing)
        {
            case ECk_Easing_Sampled::InElastic:    return EaseInElastic(X);
            case ECk_Easing_Sampled::OutElastic:   return EaseOutElastic(X);
            case ECk_Easing_Sampled::InOutElastic: return EaseInOutElastic(X);
            case ECk_Easing_Sampled::InBounce:     return EaseInBounce(X);
            case ECk_Easing_Sampled::OutBounce:    return EaseOutBounce(X);
            case ECk_Easing_Sampled::InOutBounce:  return EaseInOutBounce(X);
            default: return X;
        }
    }

    // Samples the easing into InSamples+1 cubic-auto keys over [0, 1].
    mixin void SampleEasing(UCurveFloat CurveFloat, ECk_Easing_Sampled InEasing, int InSamples)
    {
        for (auto i = 0; i <= InSamples; ++i)
        {
            const auto X = float(i) / float(InSamples);
            CurveFloat.AddCurveKey(X, EvalSampledEasing(InEasing, X));
        }
        CurveFloat.AutoSetTangents();
    }

    // https://easings.net/#easeInElastic
    asset CurveFloatEaseInElastic of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::InElastic, 128);
    }

    // https://easings.net/#easeOutElastic
    asset CurveFloatEaseOutElastic of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::OutElastic, 128);
    }

    // https://easings.net/#easeInOutElastic
    asset CurveFloatEaseInOutElastic of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::InOutElastic, 128);
    }

    // https://easings.net/#easeInBounce
    asset CurveFloatEaseInBounce of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::InBounce, 96);
    }

    // https://easings.net/#easeOutBounce
    asset CurveFloatEaseOutBounce of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::OutBounce, 96);
    }

    // https://easings.net/#easeInOutBounce
    asset CurveFloatEaseInOutBounce of UCurveFloat
    {
        SampleEasing(ECk_Easing_Sampled::InOutBounce, 96);
    }
}
