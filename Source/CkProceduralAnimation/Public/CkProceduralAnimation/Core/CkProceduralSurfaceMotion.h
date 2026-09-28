#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Templates/Function.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The ray whose hit the body last accepted, Feet for the plane through the planted feet, or Step for the top of a face
    // the body steps onto. None: no contact (contact grace or airborne). A substep that only coasts while a large turn waits
    // for confirmation accepts nothing and keeps the source.
    enum class EProceduralSurfaceContactSource : uint8
    {
        None,
        Forward,
        Down,
        LookAhead,
        Fan,
        Fall,
        Feet,
        Step
    };

    // What the body does with a face the forward ray meets that is not a step: Climb makes it the next support once
    // confirmed; Slide never makes it support and takes the travel into it away.
    enum class EProceduralSurfaceWallPolicy : uint8
    {
        Climb,
        Slide
    };

    // Wall: this substep's travel was slid along a face the body could neither step onto nor climb.
    enum class EProceduralSurfaceObstruction : uint8
    {
        None,
        Wall
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
        // 0: no stepping. Otherwise above the clearance and at most the probe reach.
        float _MaxStepHeight = 0.0f;
        EProceduralSurfaceWallPolicy _WallPolicy = EProceduralSurfaceWallPolicy::Climb;

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
        CK_PROPERTY(_MaxStepHeight);
        CK_PROPERTY(_WallPolicy);
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
        // The last substep's: Wall with the face's normal and the distance the body is kept off it while it was kept off a
        // face, None and zero otherwise. A body steered into the wall follows it on the next substep.
        EProceduralSurfaceObstruction _Obstruction = EProceduralSurfaceObstruction::None;
        FVector _ObstructionNormal = FVector::ZeroVector;
        float _ObstructionStandoff = 0.0f;

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
        CK_PROPERTY(_Obstruction);
        CK_PROPERTY(_ObstructionNormal);
        CK_PROPERTY(_ObstructionStandoff);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The support the planted feet provide: the plane (world point and normal), the frame it was fitted in and the weighted
    // feet's footprint in that frame (XY, relative to the origin), which bounds where the plane counts.
    struct CKPROCEDURALANIMATION_API FProceduralSurfaceFeetSupport
    {
        CK_GENERATED_BODY(FProceduralSurfaceFeetSupport);

    private:
        FVector _Point = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        FQuat _Basis = FQuat::Identity;
        FVector _Origin = FVector::ZeroVector;
        FVector2D _FootprintMin = FVector2D::ZeroVector;
        FVector2D _FootprintMax = FVector2D::ZeroVector;

    public:
        CK_PROPERTY(_Point);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Basis);
        CK_PROPERTY(_Origin);
        CK_PROPERTY(_FootprintMin);
        CK_PROPERTY(_FootprintMax);
    };

    // An already validated, trusted planted foot from the previous gait solve. HipLocal belongs to the simulation body;
    // the optional presentation offset is checked separately by the paced step.
    struct CKPROCEDURALANIMATION_API FProceduralSurfaceReachPaceAnchor
    {
        CK_GENERATED_BODY(FProceduralSurfaceReachPaceAnchor);

    private:
        FVector _FootWorld = FVector::ZeroVector;
        FVector _HipLocal = FVector::ZeroVector;
        float _Reach = 0.0f;

    public:
        CK_PROPERTY_GET(_FootWorld);
        CK_PROPERTY_GET(_HipLocal);
        CK_PROPERTY_GET(_Reach);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralSurfaceReachPaceAnchor, _FootWorld, _HipLocal, _Reach);
    };

    // AttemptedStanceSpeed is the maximum constrained hip displacement between the already-simulated full and zero
    // voluntary trials divided by the substep duration, not the requested travel speed. It is zero when no reach trial
    // was rejected. This includes a turn in place and excludes a command stopped equally in both trials by collision.
    // RejectedFullBody and its anchor indices describe this substep's full trial only when voluntary motion was paced,
    // time and attempted stance motion were positive, and zero motion could satisfy the authored reach bounds. Indices
    // name every anchor whose simulation or optional posed hip exceeded its normal chain reach in that same trial.
    struct CKPROCEDURALANIMATION_API FProceduralSurfaceReachPaceOutcome
    {
        CK_GENERATED_BODY(FProceduralSurfaceReachPaceOutcome);

    private:
        float _Scale = 1.0f;
        bool _PhysicalOverride = false;
        int32 _Trials = 0;
        float _AttemptedStanceSpeed = 0.0f;
        TOptional<FTransform> _RejectedFullBody;
        TArray<int32, TInlineAllocator<64>> _RejectedAnchorIndices;

    public:
        CK_PROPERTY(_Scale);
        CK_PROPERTY(_PhysicalOverride);
        CK_PROPERTY(_Trials);
        CK_PROPERTY(_AttemptedStanceSpeed);
        CK_PROPERTY(_RejectedFullBody);
        CK_PROPERTY(_RejectedAnchorIndices);
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
    // - Planted feet (InFeetSupport set): while the candidate, expressed in the support's frame, lies inside its footprint
    //   grown by a quarter clearance on every side, and the plane's normal lies within ProceduralFeetPlaneMaxAngleDegrees
    //   of the support normal, the feet give a contact on their plane straight down the support normal, at the height
    //   dot(Candidate - Point, N) / dot(Up, N) above it. It becomes the substep's down contact when it lies higher than the
    //   down ray's hit (a tie goes to the ray), so the body never rides lower than the rays put it; it carries the down
    //   ray's normal when that ray hit and the plane's normal over a miss, where the ray tells nothing about the surface.
    //   While it exists the support holds, so a face still needs confirmation, and a down miss is not a miss: the
    //   look-ahead and the fan are cast only when neither contact exists, and the grace and the fall never start. Outside
    //   the footprint every rule above applies unchanged; unset, nothing changes.
    // - Steps and walls: the forward ray, at the body's height, meets only faces taller than the clearance; a lower face is
    //   climbed by the down ray once it is under the body. With MaxStepHeight above 0, a down ray from MaxStepHeight plus
    //   a clearance above the face's hit and a quarter clearance past it, for MaxStepHeight plus two clearances, looks for
    //   the face's top: a trusted hit above the support point (the down hit, else one clearance under the body) by at most
    //   MaxStepHeight and within ConfirmAngle of the support normal is a step, which becomes the down contact (source Step)
    //   and holds the support; the face is not proposed. Any other face is a wall: Climb proposes it when the body has room
    //   on it (below); Slide never proposes it, and a wall without room is never proposed under either policy. Such a face
    //   obstructs the body: once the body would stand closer to it than the standoff (the clearance, or half the room when a
    //   body one clearance off it would stand inside the solid across the gap), the travel loses its component into it and
    //   the body is pushed back out along its normal by at most ClearanceSpeed x InStep; the down ray and the rest are cast
    //   again from there, and the state reports the obstruction (Wall, its normal, the standoff). The fan's face
    //   without room obstructs the body the same way while the travel points into it. While the last substep's
    //   obstruction is Wall and the travel points into it, one ray from the candidate along the obstruction's normal,
    //   for the probe reach, follows the wall: a hit keeps the obstruction on it, a miss ends it. The obstruction never
    //   changes the support normal or the travel tangent.
    // - Room: a contact is adopted only when a free ray from its hit along its normal, from 1 cm to one clearance off it,
    //   meets nothing or only the surface the body stands on within a quarter clearance (a concave corner).
    //   A contact without room is not proposed (the down contact under the body, else the miss path, takes its place); a
    //   pending turn with no room under the body coasts; a fall landing without room loses the velocity into its face and
    //   keeps falling, kept off the face like an obstruction. The feet contact is never checked. One ray per contact
    //   checked.
    // InVoluntaryScale in [0, 1] scales requested travel, support-frame turning and adopted-contact clearance correction.
    // It does not scale contact queries, obstruction separation, real-time confirmation/grace, or gravity. InSpeed still
    // describes steering intent for forward and fan probes even at scale zero. The caller keeps the settings valid, InStep
    // positive, the body finite and the scale finite and within [0, 1].
    CKPROCEDURALANIMATION_API auto
        StepProceduralSurfaceMotion(
            const FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InSteerDirection,
            float InSpeed,
            FCk_Time InStep,
            FProceduralSurfaceRayCast InRayCast,
            const TOptional<FProceduralSurfaceFeetSupport>& InFeetSupport,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState,
            float InVoluntaryScale = 1.0f)
        -> void;

    // Replays the same substep from its original body and support state at bounded voluntary scales. Only a simulated
    // Body+State pair is committed; zero scale still queries support, resolves obstruction and falls. A prior stance that
    // cannot fit even at zero scale is reported as a physical override and is never worsened by voluntary travel.
    CKPROCEDURALANIMATION_API auto
        StepProceduralSurfaceMotionPaced(
            const FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InSteerDirection,
            float InSpeed,
            FCk_Time InStep,
            FProceduralSurfaceRayCast InRayCast,
            const TOptional<FProceduralSurfaceFeetSupport>& InFeetSupport,
            TArrayView<const FProceduralSurfaceReachPaceAnchor> InAnchors,
            const TOptional<FTransform>& InPoseOffset,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState)
        -> FProceduralSurfaceReachPaceOutcome;
}

// --------------------------------------------------------------------------------------------------------------------
