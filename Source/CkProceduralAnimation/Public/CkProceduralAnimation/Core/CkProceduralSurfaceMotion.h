#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CoreMinimal.h"
#include "Templates/Function.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The ray whose hit the body last accepted. None: no ray hit (contact grace or airborne). A substep that only coasts while
    // a large turn waits for confirmation accepts nothing and keeps the source.
    enum class EProceduralSurfaceContactSource : uint8
    {
        None,
        Forward,
        Down,
        LookAhead,
        Fan,
        Fall
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The first hit along a queried segment. Fraction is the hit's distance along the segment over its length.
    struct CKPROCEDURALANIMATION_API FProceduralSurfaceHit
    {
        CK_GENERATED_BODY(FProceduralSurfaceHit);

    private:
        bool _Hit = false;
        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        float _Fraction = 0.0f;

    public:
        CK_PROPERTY(_Hit);
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Fraction);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralSurfaceMotionSettings
    {
        CK_GENERATED_BODY(FProceduralSurfaceMotionSettings);

    private:
        float _Clearance = 65.0f;
        float _ProbeReach = 200.0f;
        FCk_Time _ContactGrace = FCk_Time{0.12};
        float _ConfirmAngleDegrees = 30.0f;
        FCk_Time _ConfirmTime = FCk_Time{0.075};
        float _SurfaceTurnRateDegrees = 180.0f;
        float _ClearanceSpeed = 200.0f;
        FVector _Gravity = FVector{0.0, 0.0, -980.0};
        float _SteerFloor = 0.4f;

    public:
        CK_PROPERTY(_Clearance);
        CK_PROPERTY(_ProbeReach);
        CK_PROPERTY(_ContactGrace);
        CK_PROPERTY(_ConfirmAngleDegrees);
        CK_PROPERTY(_ConfirmTime);
        CK_PROPERTY(_SurfaceTurnRateDegrees);
        CK_PROPERTY(_ClearanceSpeed);
        CK_PROPERTY(_Gravity);
        CK_PROPERTY(_SteerFloor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The accepted support frame and the body's integrated motion.
    struct CKPROCEDURALANIMATION_API FProceduralSurfaceMotionState
    {
        CK_GENERATED_BODY(FProceduralSurfaceMotionState);

    private:
        FVector _SupportNormal = FVector::UpVector;
        FVector _TravelTangent = FVector::ForwardVector;
        FVector _Velocity = FVector::ZeroVector;
        FCk_Time _MissingContact = FCk_Time::ZeroSecond();
        bool _Grounded = false;
        bool _ContactTrusted = false;
        EProceduralSurfaceContactSource _ContactSource = EProceduralSurfaceContactSource::None;
        // A proposed support whose normal turns beyond the confirm angle while the current support still holds; pending
        // while _CandidateSeen is above zero.
        FVector _CandidateNormal = FVector::UpVector;
        FVector _CandidatePoint = FVector::ZeroVector;
        FCk_Time _CandidateSeen = FCk_Time::ZeroSecond();

    public:
        CK_PROPERTY(_SupportNormal);
        CK_PROPERTY(_TravelTangent);
        CK_PROPERTY(_Velocity);
        CK_PROPERTY(_MissingContact);
        CK_PROPERTY(_Grounded);
        CK_PROPERTY(_ContactTrusted);
        CK_PROPERTY(_ContactSource);
        CK_PROPERTY(_CandidateNormal);
        CK_PROPERTY(_CandidatePoint);
        CK_PROPERTY(_CandidateSeen);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Returns the first hit on the segment from InStart to InEnd.
    using FProceduralSurfaceRayCast = TFunctionRef<FProceduralSurfaceHit(const FVector& InStart, const FVector& InEnd)>;

    // Advances the body by one integration substep of InStep at InSpeed and rewrites the accepted support from the rays it
    // casts through InRayCast. A hit is trusted when it lies on the segment (fraction in (0, 1]), is finite and faces
    // against the ray.
    // - Travel: the steering direction projected onto the support plane; while that projection is shorter than
    //   SteerFloor of the steering, the travel tangent laid onto the plane.
    // - Rays: forward within the clearance; down from one clearance above the body for the probe reach; a look-ahead
    //   down ray from 1.5 clearances ahead, which counts only on a surface within 15 degrees of the support normal; and,
    //   around a convex edge, a fan back and down from one clearance ahead. The forward hit wins, then the down hit. The
    //   look-ahead replaces a down hit below the clearance when its hit lies more than a quarter clearance higher (the
    //   top the body just climbed onto, with a lower floor under it), and follows a down miss before the fan. The forward
    //   ray and the fan are cast only while InSpeed is positive; the down ray and the look-ahead at any speed.
    // - Confirmation: a contact whose normal turns more than ConfirmAngle from the support while the down ray still hits
    //   is adopted only once it has been seen for ConfirmTime, each sighting within 15 degrees of the last. Until then
    //   the body keeps the support under it, or coasts along its plane when the down ray proposed the turn, so a face
    //   the rays only graze is never adopted; a coast leaves the contact source unchanged. A body longer than its
    //   clearance can overrun a head-on wall by up to InSpeed times ConfirmTime. A down ray that hits nothing, or starts
    //   on or inside a solid, adopts any contact at once; a face it meets along its length but does not trust (grazed
    //   along the seam two solids share) still counts as support for the confirmation.
    // - Without a trusted hit the body coasts for the contact grace, then falls under gravity along a swept ray until it
    //   lands. The landing keeps the travel tangent laid onto the landing plane (then the steering, then the body's
    //   forward).
    // The caller keeps the settings valid, InStep positive and the body finite.
    CKPROCEDURALANIMATION_API auto
        StepProceduralSurfaceMotion(
            const FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InSteerDirection,
            float InSpeed,
            FCk_Time InStep,
            FProceduralSurfaceRayCast InRayCast,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState)
        -> void;
}

// --------------------------------------------------------------------------------------------------------------------
