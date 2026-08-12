#pragma once

#include "CkTween_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkSpline/CkSpline_Fragment_Data.h"

#include <Curves/CurveFloat.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Tween_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Tween_Playing);
    CK_DEFINE_ECS_TAG(FTag_Tween_Paused);
    CK_DEFINE_ECS_TAG(FTag_Tween_Completed);
    CK_DEFINE_ECS_TAG(FTag_Tween_InYoyoDelay);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Tween_Params = FCk_Fragment_Tween_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTWEEN_API FFragment_Tween_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_Current);

    private:
        float _CurrentTime = 0.0f;
        float _YoyoDelayTimer = 0.0f;
        ECk_TweenState _State = ECk_TweenState::Playing;
        int32 _CurrentLoop = 0;
        bool _IsReversed = false;
        FCk_TweenValue _CurrentValue;
        float _TimeMultiplier = 1.0f;

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Tween_Current, _CurrentValue);

        CK_PROPERTY(_CurrentTime);
        CK_PROPERTY(_YoyoDelayTimer);
        CK_PROPERTY(_State);
        CK_PROPERTY(_CurrentLoop);
        CK_PROPERTY(_IsReversed);
        CK_PROPERTY(_CurrentValue);
        CK_PROPERTY(_TimeMultiplier);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTWEEN_API FFragment_Tween_Chain
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_Chain);

    private:
        TOptional<FCk_Handle_Tween> _NextTween;

    public:
        CK_PROPERTY(_NextTween);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Spline path a tween follows. Holds an FCk_Handle_Spline (a CkSpline feature);
    // the processor resolves progress -> transform each tick via UCk_Utils_Spline_UE.
    // A destroyed spline entity cleanly stops the follower (invalid handle -> processor
    // early-outs).
    struct CKTWEEN_API FFragment_Tween_SplineFollow
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_SplineFollow);

        friend class FProcessor_Tween_ApplySplineFollow;
        friend class UCk_Utils_Tween_UE;

    private:
        FCk_Handle_Spline _Spline;
        ECk_Tween_SplineOrientation _Orientation = ECk_Tween_SplineOrientation::PositionOnly;

    public:
        CK_PROPERTY_GET(_Spline);
        CK_PROPERTY_GET(_Orientation);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Curves that PRODUCE a curve-driven tween's value, rather than reshaping the alpha between
    // Start and End. See ECk_TweenCurveOutput for why a shake needs this and easing cannot serve.
    //
    // TStrongObjectPtr and not TObjectPtr/raw: CkFoundation fragments are NOT GC-traced (see
    // CkEcs/Claude.md), so a curve reachable only from here would be collected and dangle. That
    // also rules out carrying these in FCk_Fragment_Tween_ParamsData -- it is the ECS fragment
    // itself, so a UPROPERTY on it buys no tracing either.
    struct CKTWEEN_API FFragment_Tween_CurveDrive
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_CurveDrive);

        friend class FProcessor_Tween_Update;
        friend class FProcessor_Tween_HandleRequests;
        friend class FProcessor_Tween_ApplyToTransform;
        friend class UCk_Utils_Tween_UE;

    private:
        // Maps to X/Y/Z world units under VectorOffset and Pitch/Yaw/Roll degrees under
        // RotatorOffset; X alone is the value under Float. A null channel contributes 0, so a
        // single-axis motion authors one curve and leaves the rest unset.
        TStrongObjectPtr<UCurveFloat> _Curve_X;
        TStrongObjectPtr<UCurveFloat> _Curve_Y;
        TStrongObjectPtr<UCurveFloat> _Curve_Z;

        ECk_TweenCurveOutput _Output = ECk_TweenCurveOutput::Float;
        ECk_TweenCurveTimeInput _TimeInput = ECk_TweenCurveTimeInput::ElapsedSeconds;

        // What the offset curves compose onto: the location (VectorOffset) or rotation
        // (RotatorOffset) captured when the tween starts, re-captured when a Restart arrives on
        // a NON-playing tween -- never while it is mid-flight, or the half-played value would be
        // baked in as the new base and every re-trigger would drift.
        FCk_TweenValue _BaseValue;

    public:
        CK_PROPERTY_GET(_Curve_X);
        CK_PROPERTY_GET(_Curve_Y);
        CK_PROPERTY_GET(_Curve_Z);
        CK_PROPERTY_GET(_Output);
        CK_PROPERTY_GET(_TimeInput);
        CK_PROPERTY_GET(_BaseValue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Reshapes Progress before the Start->End interpolation, replacing the ECk_TweenEasing table
    // with an authored curve. Orthogonal to FFragment_Tween_CurveDrive: this still interpolates
    // Start->End, so it does NOT enable a shake (Start == End collapses the lerp either way).
    struct CKTWEEN_API FFragment_Tween_EasingCurve
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_EasingCurve);

        friend class FProcessor_Tween_Update;
        friend class UCk_Utils_Tween_UE;

    private:
        // TStrongObjectPtr for the same reason as FFragment_Tween_CurveDrive.
        TStrongObjectPtr<UCurveFloat> _Curve;

    public:
        CK_PROPERTY_GET(_Curve);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTWEEN_API FFragment_Tween_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_Requests);

        using RequestType = std::variant<
            FCk_Request_Tween_Pause,
            FCk_Request_Tween_Resume,
            FCk_Request_Tween_Stop,
            FCk_Request_Tween_Restart,
            FCk_Request_Tween_SetTimeMultiplier
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenUpdate,
        FCk_Delegate_Tween_OnUpdate,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnUpdate);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenComplete,
        FCk_Delegate_Tween_OnComplete,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnComplete);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenLoop,
        FCk_Delegate_Tween_OnLoop,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnLoop);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Tween_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
