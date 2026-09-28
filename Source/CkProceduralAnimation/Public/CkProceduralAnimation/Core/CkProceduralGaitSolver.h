#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSwingProfile.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // A target whose ground normal lies farther than this from the support up stands on a face: it has no tread to carry a
    // foot along, so a swing lands on it where it lies (no stroke overshoot, no freeze push), no landing ground is probed
    // under it, and the gait's foothold search weighs it against the tops in reach instead of taking it outright.
    constexpr auto ProceduralGaitFaceAngleDegrees = 45.0f;

    // Coordinates are in a support-aligned frame whose +Z is up. The caller supplies
    // projected targets and rotates state with NewBasis.Inverse() * OldBasis when
    // the support frame changes. Measure body velocity in world space before rotating it.

    struct CKPROCEDURALANIMATION_API FProceduralGaitPattern
    {
        CK_GENERATED_BODY(FProceduralGaitPattern);

    private:
        friend class FProceduralGaitSolver;

        float _MinSpeed = 0.0f;
        TArray<float, TInlineAllocator<8>> _PhaseOffsets;
        float _CycleDurationScale = 1.0f;

    public:
        CK_PROPERTY(_MinSpeed);
        CK_PROPERTY(_PhaseOffsets);
        CK_PROPERTY(_CycleDurationScale);
    };

    // --------------------------------------------------------------------------------------------------------------------

    enum class EProceduralGaitLegLossPolicy : uint8
    {
        KeepAuthoredOffsets,
        RedistributeOffsets
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitCadenceSettings
    {
        CK_GENERATED_BODY(FProceduralGaitCadenceSettings);

    private:
        friend class FProceduralGaitSolver;

        FCk_Time _CycleDuration = FCk_Time{0.5};
        float _SwingWindow = 0.5f;
        float _MoveSpeedThreshold = 5.0f;
        float _CadenceSpeedRef = 0.0f;
        float _MaxCadenceScale = 3.0f;
        int32 _MaxSimultaneousSwings = 0;

    public:
        CK_PROPERTY(_CycleDuration);
        CK_PROPERTY(_SwingWindow);
        CK_PROPERTY(_MoveSpeedThreshold);
        CK_PROPERTY(_CadenceSpeedRef);
        CK_PROPERTY(_MaxCadenceScale);
        CK_PROPERTY(_MaxSimultaneousSwings);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitStepSettings
    {
        CK_GENERATED_BODY(FProceduralGaitStepSettings);

    private:
        friend class FProceduralGaitSolver;

        float _Threshold = 25.0f;
        float _EmergencyFactor = 1.75f;
        FCk_Time _Duration = FCk_Time{0.25};
        float _RetargetSmoothing = 14.0f;
        float _RetargetFreezePhase = 0.7f;
        float _StrokeOvershootFraction = 0.25f;
        float _MaxStrokeOvershoot = 20.0f;

    public:
        CK_PROPERTY(_Threshold);
        CK_PROPERTY(_EmergencyFactor);
        CK_PROPERTY(_Duration);
        CK_PROPERTY(_RetargetSmoothing);
        CK_PROPERTY(_RetargetFreezePhase);
        CK_PROPERTY(_StrokeOvershootFraction);
        CK_PROPERTY(_MaxStrokeOvershoot);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitSwingSettings
    {
        CK_GENERATED_BODY(FProceduralGaitSwingSettings);

    private:
        friend class FProceduralGaitSolver;

        float _Height = 15.0f;
        float _ApexPhase = 0.5f;
        float _ApexSharpness = 1.0f;
        float _SprintApexHeightScale = 0.65f;
        float _ObstacleClearance = 6.0f;
        FProceduralGaitSwingProfile _Profile;
        float _ToePitchDegrees = 0.0f;

    public:
        CK_PROPERTY(_Height);
        CK_PROPERTY(_ApexPhase);
        CK_PROPERTY(_ApexSharpness);
        CK_PROPERTY(_SprintApexHeightScale);
        CK_PROPERTY(_ObstacleClearance);
        CK_PROPERTY(_Profile);
        CK_PROPERTY(_ToePitchDegrees);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitScheduleSettings
    {
        CK_GENERATED_BODY(FProceduralGaitScheduleSettings);

    private:
        friend class FProceduralGaitSolver;

        float _AdvanceFraction = 0.6f;
        float _AdvanceRate = 1.5f;
        FCk_Time _CatchStepLifetime = FCk_Time{0.0};

    public:
        CK_PROPERTY(_AdvanceFraction);
        CK_PROPERTY(_AdvanceRate);
        CK_PROPERTY(_CatchStepLifetime);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitSettleSettings
    {
        CK_GENERATED_BODY(FProceduralGaitSettleSettings);

    private:
        friend class FProceduralGaitSolver;

        bool _AtRest = true;
        FCk_Time _Delay = FCk_Time{0.35};
        float _ThresholdFraction = 0.35f;

    public:
        CK_PROPERTY(_AtRest);
        CK_PROPERTY(_Delay);
        CK_PROPERTY(_ThresholdFraction);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitAirborneSettings
    {
        CK_GENERATED_BODY(FProceduralGaitAirborneSettings);

    private:
        friend class FProceduralGaitSolver;

        float _TuckLift = 15.0f;
        float _FollowSpeed = 8.0f;
        float _LandingStepDurationScale = 0.5f;

    public:
        CK_PROPERTY(_TuckLift);
        CK_PROPERTY(_FollowSpeed);
        CK_PROPERTY(_LandingStepDurationScale);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Fractions of a leg's reach. Swing targets stay within _TargetFraction of the hip, a planted foot beyond
    // _ForceStepFraction is an Emergency, and one beyond _HardOverstretchFraction may step beyond the phase schedule.
    // _TouchdownLiftFraction is a fraction of the swing height instead: a report of higher ground under the landing point
    // that first arrives at touchdown lifts the plant at most this share of the step height (2 cm at a 25 cm step); a
    // larger rise would read as a pop, so it is left and counted.
    struct CKPROCEDURALANIMATION_API FProceduralGaitReachSettings
    {
        CK_GENERATED_BODY(FProceduralGaitReachSettings);

    private:
        friend class FProceduralGaitSolver;

        float _TargetFraction = 0.8f;
        float _ForceStepFraction = 0.92f;
        float _HardOverstretchFraction = 1.0f;
        float _TouchdownLiftFraction = 0.08f;

    public:
        CK_PROPERTY(_TargetFraction);
        CK_PROPERTY(_ForceStepFraction);
        CK_PROPERTY(_HardOverstretchFraction);
        CK_PROPERTY(_TouchdownLiftFraction);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitPatternSettings
    {
        CK_GENERATED_BODY(FProceduralGaitPatternSettings);

    private:
        friend class FProceduralGaitSolver;

        TArray<FProceduralGaitPattern> _Patterns;
        FCk_Time _BlendTime = FCk_Time{0.4};
        float _SwitchHysteresis = 0.85f;
        EProceduralGaitLegLossPolicy _LegLossPolicy = EProceduralGaitLegLossPolicy::KeepAuthoredOffsets;

    public:
        CK_PROPERTY(_Patterns);
        CK_PROPERTY(_BlendTime);
        CK_PROPERTY(_SwitchHysteresis);
        CK_PROPERTY(_LegLossPolicy);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitSettings
    {
        CK_GENERATED_BODY(FProceduralGaitSettings);

    private:
        friend class FProceduralGaitSolver;

        FProceduralGaitCadenceSettings _Cadence;
        FProceduralGaitStepSettings _Step;
        FProceduralGaitSwingSettings _Swing;
        FProceduralGaitScheduleSettings _Schedule;
        FProceduralGaitSettleSettings _Settle;
        FProceduralGaitAirborneSettings _Airborne;
        FProceduralGaitPatternSettings _Pattern;
        FProceduralGaitReachSettings _Reach;

    public:
        CK_PROPERTY(_Cadence);
        CK_PROPERTY(_Step);
        CK_PROPERTY(_Swing);
        CK_PROPERTY(_Schedule);
        CK_PROPERTY(_Settle);
        CK_PROPERTY(_Airborne);
        CK_PROPERTY(_Pattern);
        CK_PROPERTY(_Reach);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // What the caller's probe under a swing's landing point (FProceduralGaitLegSwing::_LandingPoint) found. Unknown: no
    // probe was cast (a planted leg, a catch step, a target on a face). None: the probe found no trusted ground within the
    // force-step reach. Found: ground, at the height the input's _LandingGroundZ carries.
    enum class EProceduralGaitLandingGround : uint8
    {
        Unknown,
        None,
        Found
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralFootReservation
    {
        CK_GENERATED_BODY(FProceduralFootReservation);

    private:
        FVector _Position = FVector::ZeroVector;
        float _Radius = 0.0f;
        int32 _LegIndex = INDEX_NONE;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_LegIndex);
        CK_DEFINE_CONSTRUCTORS(FProceduralFootReservation, _Position, _Radius, _LegIndex);
    };

    // Positions share one frame and radii are finite nonnegative centimetres. Radius zero opts out; only positive-radius
    // reservations belonging to another leg obstruct a contact. Tangency is available. Malformed active records reject.
    CKPROCEDURALANIMATION_API auto
        Get_IsProceduralFootContactAvailable(
            const FVector& InPosition,
            float InRadius,
            int32 InLegIndex,
            TArrayView<const FProceduralFootReservation> InReservations)
        -> bool;

    // --------------------------------------------------------------------------------------------------------------------

    // The same full voluntary-motion trial that rejected this leg's trusted plant. Positions use the input's support
    // frame. The caller supplies a fresh trial only for the unchanged live plant; it grants release only if the current
    // trusted replacement fits every trial hip while the old plant does not. It never changes query or target geometry.
    struct CKPROCEDURALANIMATION_API FProceduralGaitReachPaceTrial
    {
        CK_GENERATED_BODY(FProceduralGaitReachPaceTrial);

    private:
        FVector _Hip = FVector::ZeroVector;
        TOptional<FVector> _PosedHip;

    public:
        CK_PROPERTY(_Hip);
        CK_PROPERTY(_PosedHip);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // _Hip is the simulation hip in the support frame like every other position. When set, _PosedHip
    // contributes only to planted reach urgency; target validity, query geometry and reach clamping keep using _Hip.
    // _Reach is the leg's chain length in centimetres; zero disables the reach clamp and the reach Emergency for that leg.
    // _LandingGround reports the ground under the swing's
    // landing point and _LandingGroundZ, meaningful only when it is Found, its support-frame height: a swing whose landing
    // ground lies above its target, within reach, lifts the rest of its arc onto it; a swing told None lands on the target
    // the caller validated instead of past it, and plants untrusted when None is still the report at touchdown. _PlantOccluded
    // marks a planted foot whose hip-to-foot line passes through a solid: with a valid target it is an Emergency of
    // ratio 1 that steps beyond the schedule. _TargetIsFoothold marks a target the caller chose and validated as a spot to
    // stand on: a swing aimed at it gets neither the stroke overshoot nor the freeze push, so it lands on that spot. A swing
    // whose target is on a face when it accepts it is treated the same way (FProceduralGaitLegSwing::_TargetOnAFace).
    // _TargetTrusted marks a target on ground the caller probed and trusts. The solver never moves such a target off its
    // ground: while it lies within the force-step reach of the hip it is not held to the target reach, and beyond the
    // force-step reach it counts as no target. At touchdown the plant is trusted when the landing ground is Found,
    // untrusted when it is None, and otherwise when the swing's recorded target was trusted (FProceduralGaitLegPlant::_Trusted).
    // Positive _FootContactRadius reserves exact trusted landing geometry against other enabled positive-radius contacts.
    // _PlantCrowded requests a budgeted Emergency toward a valid replacement without moving the existing plant. Radius zero
    // retains displacement and reservation behavior of existing callers; radii must be finite and nonnegative.
    struct CKPROCEDURALANIMATION_API FProceduralGaitLegInput
    {
        CK_GENERATED_BODY(FProceduralGaitLegInput);

    private:
        friend class FProceduralGaitSolver;

        FVector _IdealTarget = FVector::ZeroVector;
        FVector _GroundNormal = FVector::UpVector;
        float _PhaseOffset = 0.0f;
        float _StepThresholdScale = 1.0f;
        FVector _FacingDirection = FVector::ForwardVector;
        bool _TargetValid = true;
        float _ClearanceGroundZ = -FLT_MAX;
        bool _Enabled = true;
        FVector _Hip = FVector::ZeroVector;
        TOptional<FVector> _PosedHip;
        float _Reach = 0.0f;
        EProceduralGaitLandingGround _LandingGround = EProceduralGaitLandingGround::Unknown;
        float _LandingGroundZ = 0.0f;
        bool _PlantOccluded = false;
        bool _TargetIsFoothold = false;
        bool _TargetTrusted = true;
        float _FootContactRadius = 0.0f;
        bool _PlantCrowded = false;
        TOptional<FProceduralGaitReachPaceTrial> _ReachPaceTrial;

    public:
        CK_PROPERTY(_IdealTarget);
        CK_PROPERTY(_GroundNormal);
        CK_PROPERTY(_PhaseOffset);
        CK_PROPERTY(_StepThresholdScale);
        CK_PROPERTY(_FacingDirection);
        CK_PROPERTY(_TargetValid);
        CK_PROPERTY(_ClearanceGroundZ);
        CK_PROPERTY(_Enabled);
        CK_PROPERTY(_Hip);
        CK_PROPERTY(_PosedHip);
        CK_PROPERTY(_Reach);
        CK_PROPERTY(_LandingGround);
        CK_PROPERTY(_LandingGroundZ);
        CK_PROPERTY(_PlantOccluded);
        CK_PROPERTY(_TargetIsFoothold);
        CK_PROPERTY(_TargetTrusted);
        CK_PROPERTY(_FootContactRadius);
        CK_PROPERTY(_PlantCrowded);
        CK_PROPERTY(_ReachPaceTrial);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegOutput
    {
        CK_GENERATED_BODY(FProceduralGaitLegOutput);

    private:
        friend class FProceduralGaitSolver;

        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        FQuat _Rotation = FQuat::Identity;
        float _SwingAlpha = 0.0f;
        bool _Planted = true;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Rotation);
        CK_PROPERTY(_SwingAlpha);
        CK_PROPERTY(_Planted);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // _Trusted is whether the foot touched down on ground the caller trusted (see FProceduralGaitLegInput::_TargetTrusted);
    // it holds until the next touchdown. A disabled leg's frozen pose rides with the body and is never trusted.
    struct CKPROCEDURALANIMATION_API FProceduralGaitLegPlant
    {
        CK_GENERATED_BODY(FProceduralGaitLegPlant);

    private:
        friend class FProceduralGaitSolver;

        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        FQuat _Rotation = FQuat::Identity;
        bool _Trusted = true;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Rotation);
        CK_PROPERTY(_Trusted);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // _LandingPoint is where the swing will touch down, after the stroke overshoot and the reach clamp: on the take-off frame,
    // the swing target; before the retarget freeze, the point the freeze will produce from this frame's ideal target; from the
    // freeze on, the actual landing point.
    // _LiftedLandingPoint is the landing target raised onto the ground reported under it, and _LandingLiftStartAlpha the
    // swing alpha the lift began at (negative while the swing is not lifted): the remaining arc rises onto the lifted point
    // in step with the swing's own easing. _BeyondSchedule marks a swing a hard-overstretched or occluded foot began outside
    // the phase schedule; it neither holds back nor waits for the schedule's take-offs. _Overshoot is cleared for good once
    // the swing is aimed at a foothold before its freeze, so the landing point cannot flip back and forth with the input.
    // _ValidatedTarget is the target the caller handed over, captured once: the swing target at take-off, then the ideal
    // target the freeze took before its push. _LandsOnTarget marks a swing told before its freeze that no ground lies under
    // its landing point: it keeps neither the overshoot nor the freeze push, so it lands on the validated target. A swing
    // first told so after its freeze is pulled back from _PullBackFrom to _ValidatedTarget over the rest of its swing, in
    // step with its own easing from the swing alpha _PullBackStartAlpha (negative while not pulled back). _TargetTrusted
    // and _TargetNormal are the trust and ground normal of the target as last accepted (at take-off, on a retarget before the
    // freeze, at the freeze); an input with no valid target leaves them, and the target, as they are. _TargetOnAFace marks a
    // swing whose target was on a face (its ground normal farther than ProceduralGaitFaceAngleDegrees from the support up)
    // when the swing accepted it, and holds for the rest of the swing: the support frame can pitch while the foot is in the
    // air, and a face read as ground mid-swing would take a displacement and a lift meant for a tread.
    struct CKPROCEDURALANIMATION_API FProceduralGaitLegSwing
    {
        CK_GENERATED_BODY(FProceduralGaitLegSwing);

    private:
        friend class FProceduralGaitSolver;

        FVector _StartPosition = FVector::ZeroVector;
        FQuat _StartRotation = FQuat::Identity;
        FVector _Target = FVector::ZeroVector;
        float _Phase = 0.0f;
        float _DurationScale = 1.0f;
        bool _Active = false;
        bool _TargetFrozen = false;
        bool _Overshoot = false;
        bool _CatchStep = false;
        FVector _LandingPoint = FVector::ZeroVector;
        FVector _LiftedLandingPoint = FVector::ZeroVector;
        float _LandingLiftStartAlpha = -1.0f;
        bool _BeyondSchedule = false;
        FVector _ValidatedTarget = FVector::ZeroVector;
        bool _LandsOnTarget = false;
        FVector _PullBackFrom = FVector::ZeroVector;
        float _PullBackStartAlpha = -1.0f;
        bool _TargetTrusted = true;
        FVector _TargetNormal = FVector::UpVector;
        bool _TargetOnAFace = false;

    public:
        CK_PROPERTY(_StartPosition);
        CK_PROPERTY(_StartRotation);
        CK_PROPERTY(_Target);
        CK_PROPERTY(_Phase);
        CK_PROPERTY(_DurationScale);
        CK_PROPERTY(_Active);
        CK_PROPERTY(_TargetFrozen);
        CK_PROPERTY(_Overshoot);
        CK_PROPERTY(_CatchStep);
        CK_PROPERTY(_LandingPoint);
        CK_PROPERTY(_LiftedLandingPoint);
        CK_PROPERTY(_LandingLiftStartAlpha);
        CK_PROPERTY(_BeyondSchedule);
        CK_PROPERTY(_ValidatedTarget);
        CK_PROPERTY(_LandsOnTarget);
        CK_PROPERTY(_PullBackFrom);
        CK_PROPERTY(_PullBackStartAlpha);
        CK_PROPERTY(_TargetTrusted);
        CK_PROPERTY(_TargetNormal);
        CK_PROPERTY(_TargetOnAFace);

        // The forecast landing XY and accepted lift height. A later admitted report may remove the lift but cannot lower
        // the landing below its target. Its XY follows a pullback rather than retaining where a prior lift began.
        auto
            Get_CommittedLandingPoint() const
            -> FVector;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegPendingStep
    {
        CK_GENERATED_BODY(FProceduralGaitLegPendingStep);

    private:
        friend class FProceduralGaitSolver;

        FVector _Target = FVector::ZeroVector;
        FCk_Time _Time = FCk_Time{0.0};

    public:
        CK_PROPERTY(_Target);
        CK_PROPERTY(_Time);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Transitions start from the pose that was emitted, including clearance and authored swing shape.
    struct CKPROCEDURALANIMATION_API FProceduralGaitLegPose
    {
        CK_GENERATED_BODY(FProceduralGaitLegPose);

    private:
        friend class FProceduralGaitSolver;

        FVector _Position = FVector::ZeroVector;
        FQuat _Rotation = FQuat::Identity;
        FVector _AirPosition = FVector::ZeroVector;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Rotation);
        CK_PROPERTY(_AirPosition);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegState
    {
        CK_GENERATED_BODY(FProceduralGaitLegState);

    private:
        friend class FProceduralGaitSolver;

        FProceduralGaitLegPlant _Plant;
        FProceduralGaitLegSwing _Swing;
        FProceduralGaitLegPendingStep _PendingStep;
        FProceduralGaitLegPose _Emitted;
        bool _Enabled = true;

    public:
        CK_PROPERTY(_Plant);
        CK_PROPERTY(_Swing);
        CK_PROPERTY(_PendingStep);
        CK_PROPERTY_GET(_Emitted);
        CK_PROPERTY_GET(_Enabled);
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProceduralGaitVelocityTracker
    {
        CK_GENERATED_BODY(FProceduralGaitVelocityTracker);

    public:
        auto Update(const FVector& InWorldPosition, FCk_Time InDeltaTime) -> FVector;
        auto GetVelocity() const -> FVector;

    private:
        static constexpr auto MaxSamples = int32{5};
        TArray<FVector, TInlineAllocator<MaxSamples>> _Samples;
        FVector _PreviousPosition = FVector::ZeroVector;
        int32 _NextSample = 0;
        bool _HasPrevious = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The body's turn rate about its up, in radians per second, signed right-handed about the previous basis's +Z. Only the
    // twist part of the basis's frame-to-frame rotation counts, so a tilt step (a facet, a wall corner) reads as no turn;
    // the rate is low-passed with a time constant of SmoothingTime.
    class CKPROCEDURALANIMATION_API FProceduralGaitYawRateTracker
    {
        CK_GENERATED_BODY(FProceduralGaitYawRateTracker);

    public:
        static constexpr auto SmoothingTime = FCk_Time{0.1};

    public:
        auto Update(const FQuat& InPreviousBasis, const FQuat& InBasis, FCk_Time InDeltaTime) -> float;
        auto GetYawRate() const -> float { return _YawRate; }

    private:
        float _YawRate = 0.0f;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGaitPatternBlendState
    {
        CK_GENERATED_BODY(FProceduralGaitPatternBlendState);

    private:
        friend class FProceduralGaitSolver;

        int32 _CurrentIndex = INDEX_NONE;
        float _Alpha = 1.0f;
        float _EffectiveCycleScale = 1.0f;
        float _FromCycleScale = 1.0f;
        TArray<float, TInlineAllocator<8>> _EffectiveOffsets;
        TArray<float, TInlineAllocator<8>> _FromOffsets;
        TArray<float, TInlineAllocator<8>> _RedistributedOffsets;
        bool _HasRedistributedOffsets = false;

    public:
        CK_PROPERTY(_CurrentIndex);
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_EffectiveCycleScale);
        CK_PROPERTY(_FromCycleScale);
        CK_PROPERTY(_EffectiveOffsets);
        CK_PROPERTY(_FromOffsets);
        CK_PROPERTY(_RedistributedOffsets);
        CK_PROPERTY(_HasRedistributedOffsets);
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPROCEDURALANIMATION_API FProceduralGaitSolver
    {
        CK_GENERATED_BODY(FProceduralGaitSolver);

    public:
        // Rejected input never changes state or the caller's output array. InInitialFootTrusted says, per foot, whether it
        // stands on ground the caller trusts; empty trusts every foot, any other count than the positions' is rejected.
        auto Reset(TArrayView<const FVector> InInitialFootPositions, TArrayView<const bool> InInitialFootTrusted = {}) -> bool;
        // Optional cadence drive lets a body paced by planted reach keep opening phase windows. Pattern selection and
        // at-rest settling still use measured InBodyPlanarSpeed; target prediction still uses InBodyPlanarVelocity.
        auto Step(FCk_Time InDeltaTime, float InBodyPlanarSpeed, const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            TArrayView<FProceduralGaitLegOutput> OutOutputs, bool InAirborne = false,
            TOptional<float> InCadenceDriveSpeed = {}) -> bool;
        auto NumLegs() const -> int32 { return _LegStates.Num(); }
        auto IsLegEnabled(int32 InLegIndex) const -> bool;
        auto NumEnabledLegs() const -> int32;
        auto GetGaitClock() const -> float { return _GaitClock; }
        auto IsWindowOpen(float InPhaseOffset) const -> bool;
        auto GetLegState(int32 InLegIndex) const -> const FProceduralGaitLegState& { return _LegStates[InLegIndex]; }
        auto GetEffectivePhaseOffset(int32 InLegIndex) const -> float
        {
            return _PatternBlend._EffectiveOffsets.IsValidIndex(InLegIndex) ? _PatternBlend._EffectiveOffsets[InLegIndex] : 0.0f;
        }
        auto GetCurrentPatternIndex() const -> int32 { return _PatternBlend._CurrentIndex; }
        auto IsAirborne() const -> bool { return _WasAirborne; }
        auto GetRestTime() const -> FCk_Time { return _RestTime; }
        auto SetPlantedPose(int32 InLegIndex, const FVector& InPosition, const FVector& InNormal, bool InTrusted = true) -> void;
        auto SetDisabledPose(int32 InLegIndex, const FVector& InPosition, const FQuat& InRotation,
            const FVector& InNormal) -> bool;
        auto RequestStep(int32 InLegIndex, const FVector& InTarget) -> bool;
        auto HasPendingStep(int32 InLegIndex) const -> bool
        {
            return _LegStates.IsValidIndex(InLegIndex) && _LegStates[InLegIndex]._PendingStep._Time > FCk_Time{};
        }
        auto TransformState(const FQuat& InDelta) -> void;
        static auto MakeFootRotation(const FVector& InFacingDirection, const FVector& InGroundNormal) -> FQuat;
        static auto ComputeTraceAxis(const FVector& InBodyUp, const FVector& InRadialDirection, float InOutwardLean) -> FVector;
        static auto ValidateSettings(const FProceduralGaitSettings& InSettings) -> bool;
        // Whether InNormal lies farther than ProceduralGaitFaceAngleDegrees from InUp; a zero normal is a face.
        static auto Get_IsFaceNormal(const FVector& InNormal, const FVector& InUp) -> bool;
        // Keeps InTarget within InMaxDistance of InHip: the planar (X, Y) offset shrinks and the height is kept; a height
        // alone beyond the limit clamps in 3D. A non-positive limit leaves the target unchanged.
        static auto ClampToReach(const FVector& InHip, const FVector& InTarget, float InMaxDistance) -> FVector;
        // Raibert placement: InNeutral moved by InLinearLead, then turned about InPivot, around InUp, through the angle a body
        // turning at InYawRate (radians per second) covers in InLeadTime, so a moving, turning body's feet land ahead of it.
        // The result leads InNeutral by at most InMaxLead, whatever the mix of travel and turn.
        static auto ComputeLeadQuery(const FVector& InNeutral, const FVector& InLinearLead, const FVector& InPivot,
            const FVector& InUp, float InYawRate, FCk_Time InLeadTime, float InMaxLead) -> FVector;

    private:
        auto DoStep(FCk_Time InDeltaTime, float InBodyPlanarSpeed, float InCadenceDriveSpeed,
            const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs, TArrayView<FProceduralGaitLegOutput> OutOutputs,
            bool InAirborne) -> bool;
        auto UpdatePatternSelection(FCk_Time InDeltaTime, float InBodyPlanarSpeed,
            TArrayView<const FProceduralGaitLegInput> InInputs, bool InAdvance) -> void;
        auto DoSelectPattern(float InBodyPlanarSpeed, bool InAdvance) -> void;
        auto DoReconcileEnabled(FProceduralGaitLegState& InOutState, const FProceduralGaitLegInput& InInput) const -> bool;
        auto DoClampToReach(const FProceduralGaitLegInput& InInput, const FVector& InTarget) const -> FVector;
        auto DoClampTarget(const FProceduralGaitLegInput& InInput, const FVector& InTarget, bool InTrusted) const -> FVector;
        auto DoGet_DisplacedTarget(const FProceduralGaitLegInput& InInput, const FVector& InTarget, const FVector& InDisplacement,
            bool InTrusted) const -> FVector;
        auto DoGet_EffectiveInput(const FProceduralGaitLegInput& InInput) const -> FProceduralGaitLegInput;
        auto DoGet_IsContactAvailable(int32 InLegIndex, const FVector& InPosition,
            TArrayView<const FProceduralGaitLegInput> InInputs) const -> bool;
        static auto DoGet_PlantReachDistance(const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput) -> double;
        static auto DoGet_IsReachPaceRelief(const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput) -> bool;
        auto DoGet_IsEmergency(const FProceduralGaitLegState& InState, const FProceduralGaitLegInput& InInput) const -> bool;
        auto DoGet_IsHardOverstretched(const FProceduralGaitLegState& InState, const FProceduralGaitLegInput& InInput) const -> bool;
        auto DoGet_EmergencyRatio(const FProceduralGaitLegState& InState, const FProceduralGaitLegInput& InInput) const -> double;
        auto DoRedistributeOffsets() -> void;
        auto DoClearRedistribution() -> void;
        auto DoGet_SwingEase(float InPhase) const -> float;
        auto DoGet_FrozenTarget(const FProceduralGaitLegState& InState, const FProceduralGaitLegInput& InInput,
            const FVector& InBodyPlanarVelocity, FCk_Time InRemainingTime) const -> FVector;
        auto DoGet_LandingPoint(const FProceduralGaitLegState& InState, const FProceduralGaitLegInput& InInput,
            const FVector& InTarget, const FVector& InSwingTarget) const -> FVector;
        static auto DoBeginSwing(FProceduralGaitLegState& InOutState, const FVector& InStartPosition,
            const FQuat& InStartRotation, const FVector& InTarget, bool InTargetTrusted, const FVector& InTargetNormal) -> void;
        static auto DoWriteDisabledOutput(const FProceduralGaitLegState& InState, FProceduralGaitLegOutput& OutOutput) -> void;

    private:
        FProceduralGaitSettings _Settings;
        TArray<FProceduralGaitLegState> _LegStates;
        float _GaitClock = 0.0f;
        float _LastCadenceScale = 1.0f;
        FCk_Time _RestTime;
        bool _WasAirborne = false;
        FProceduralGaitPatternBlendState _PatternBlend;
        // Touchdowns whose landing ground lay more than the touchdown safety net above the target and whose swing was never
        // lifted onto it (the ground report arrived too late); each planted below that ground.
        int32 _MissedLandingLifts = 0;

    public:
        CK_PROPERTY(_Settings);
        CK_PROPERTY_GET(_LastCadenceScale);
        CK_PROPERTY_GET(_MissedLandingLifts);
    };
}

// --------------------------------------------------------------------------------------------------------------------
