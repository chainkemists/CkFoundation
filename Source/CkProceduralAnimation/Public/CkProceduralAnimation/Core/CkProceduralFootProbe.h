#pragma once

#include "CkProceduralAnimation/Core/CkProceduralSurfaceMotion.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class EProceduralFootProbeState : uint8
    {
        Grounded,
        Guessing,
        Lost
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralFootProbeState
    {
        CK_GENERATED_BODY(FProceduralFootProbeState);

    public:
        // Grace is elapsed simulation time, independent of the number of probe updates.
        // Zero delta preserves the state. Invalid durations return false without mutation.
        auto Advance(bool InHit, FCk_Time InDeltaTime, FCk_Time InGraceDuration) -> bool;
        auto Reset() -> void;

    private:
        FCk_Time _MissingDuration;
        EProceduralFootProbeState _State = EProceduralFootProbeState::Grounded;

    public:
        CK_PROPERTY_GET(_MissingDuration);
        CK_PROPERTY_GET(_State);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGroundProbeSpan
    {
        CK_GENERATED_BODY(FProceduralGroundProbeSpan);

    private:
        float _UpDistance = 0.0f;
        float _DownDistance = 0.0f;
        float _OutwardLean = 0.0f;

    public:
        CK_PROPERTY(_UpDistance);
        CK_PROPERTY(_DownDistance);
        CK_PROPERTY(_OutwardLean);
    };

    // --------------------------------------------------------------------------------------------------------------------

    inline constexpr auto ProceduralGroundProbeAttempts = int32{3};

    CKPROCEDURALANIMATION_API auto
        MakeProceduralGroundProbeSpan(
            int32 InAttempt,
            float InUpDistance,
            float InDownDistance,
            float InOutwardLean)
        -> FProceduralGroundProbeSpan;

    // --------------------------------------------------------------------------------------------------------------------

    // Where a touchdown stands, how it lies and whether ground confirmed it, and how many rays the check cast.
    struct CKPROCEDURALANIMATION_API FProceduralTouchdown
    {
        CK_GENERATED_BODY(FProceduralTouchdown);

    private:
        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        bool _Trusted = false;
        int32 _Rays = 0;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Trusted);
        CK_PROPERTY(_Rays);
    };

    // A touchdown is confirmed with one ray through the plant along the normal of the ground the swing aimed at, from
    // InHalfSpan in front of the plant to InHalfSpan behind it. A trusted hit (on the segment, finite, facing against the ray)
    // re-plants the foot at the hit, with the hit's normal, so a foot never stands off the surface that confirmed it. On a
    // miss the same ray is cast once more through InValidatedTarget, when that lies elsewhere, and a hit there re-plants the
    // foot at that hit. A second miss leaves the plant where it landed, untrusted, with InUp: a ray that met nothing tells
    // nothing about the surface.
    CKPROCEDURALANIMATION_API auto
        ResolveProceduralTouchdown(
            const FVector& InPlant,
            const FVector& InValidatedTarget,
            const FVector& InTargetNormal,
            const FVector& InUp,
            float InHalfSpan,
            FProceduralSurfaceRayCast InRayCast)
        -> FProceduralTouchdown;

    // A confirmed surface is trusted only when both the simulation hip and, when supplied, the ready presentation hip can
    // reach the hit with the leg's full physical chain. A rejected first hit still permits the validated-target ray.
    // InSimulationHip and InPresentationHip are finite world positions; InReach is finite and positive.
    CKPROCEDURALANIMATION_API auto
        ResolveProceduralTouchdown(
            const FVector& InPlant,
            const FVector& InValidatedTarget,
            const FVector& InTargetNormal,
            const FVector& InUp,
            float InHalfSpan,
            FProceduralSurfaceRayCast InRayCast,
            const FVector& InSimulationHip,
            const TOptional<FVector>& InPresentationHip,
            float InReach)
        -> FProceduralTouchdown;

    // Additional admission receives each actual trusted hit synchronously; it is never retained. A rejected original hit
    // still permits the validated-target ray. If neither hit meets reach and admission, retain the original plant untrusted.
    CKPROCEDURALANIMATION_API auto
        ResolveProceduralTouchdown(
            const FVector& InPlant,
            const FVector& InValidatedTarget,
            const FVector& InTargetNormal,
            const FVector& InUp,
            float InHalfSpan,
            FProceduralSurfaceRayCast InRayCast,
            const FVector& InSimulationHip,
            const TOptional<FVector>& InPresentationHip,
            float InReach,
            TFunctionRef<bool(const FVector&)> InIsContactAvailable)
        -> FProceduralTouchdown;
}

// --------------------------------------------------------------------------------------------------------------------
