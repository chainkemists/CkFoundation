#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSwingProfile.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

    public:
        CK_PROPERTY(_Cadence);
        CK_PROPERTY(_Step);
        CK_PROPERTY(_Swing);
        CK_PROPERTY(_Schedule);
        CK_PROPERTY(_Settle);
        CK_PROPERTY(_Airborne);
        CK_PROPERTY(_Pattern);
    };

    // --------------------------------------------------------------------------------------------------------------------

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

    public:
        CK_PROPERTY(_IdealTarget);
        CK_PROPERTY(_GroundNormal);
        CK_PROPERTY(_PhaseOffset);
        CK_PROPERTY(_StepThresholdScale);
        CK_PROPERTY(_FacingDirection);
        CK_PROPERTY(_TargetValid);
        CK_PROPERTY(_ClearanceGroundZ);
        CK_PROPERTY(_Enabled);
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

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegPlant
    {
        CK_GENERATED_BODY(FProceduralGaitLegPlant);

    private:
        friend class FProceduralGaitSolver;

        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        FQuat _Rotation = FQuat::Identity;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Rotation);
    };

    // --------------------------------------------------------------------------------------------------------------------

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
        auto Reset() -> void;
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
        // Rejected input never changes state or the caller's output array.
        auto Reset(TArrayView<const FVector> InInitialFootPositions) -> bool;
        auto Step(FCk_Time InDeltaTime, float InBodyPlanarSpeed, const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            TArrayView<FProceduralGaitLegOutput> OutOutputs, bool InAirborne = false) -> bool;
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
        auto SetPlantedPose(int32 InLegIndex, const FVector& InPosition, const FVector& InNormal) -> void;
        auto SetDisabledPose(int32 InLegIndex, const FVector& InPosition, const FQuat& InRotation,
            const FVector& InNormal) -> bool;
        auto RequestStep(int32 InLegIndex, const FVector& InTarget) -> bool;
        auto HasPendingStep(int32 InLegIndex) const -> bool
        {
            return _LegStates.IsValidIndex(InLegIndex) && _LegStates[InLegIndex]._PendingStep._Time > FCk_Time{};
        }
        auto ClearPendingStep(int32 InLegIndex) -> void
        {
            if (_LegStates.IsValidIndex(InLegIndex))
            { _LegStates[InLegIndex]._PendingStep._Time = FCk_Time{}; }
        }
        auto TransformState(const FQuat& InDelta) -> void;
        static auto MakeFootRotation(const FVector& InFacingDirection, const FVector& InGroundNormal) -> FQuat;
        static auto ComputeTraceAxis(const FVector& InBodyUp, const FVector& InRadialDirection, float InOutwardLean) -> FVector;
        static auto WrapLerpClock(float InCurrent, float InTarget, float InAlpha) -> float
        {
            const auto Delta = FMath::Frac(InTarget - InCurrent + 1.5f) - 0.5f;
            return FMath::Frac(InCurrent + Delta * FMath::Clamp(InAlpha, 0.0f, 1.0f) + 1.0f);
        }
        auto NudgeClock(float InTargetClock, float InAlpha) -> void
        { _GaitClock = WrapLerpClock(_GaitClock, InTargetClock, InAlpha); }
        static auto ValidateSettings(const FProceduralGaitSettings& InSettings) -> bool;

    private:
        auto DoStep(FCk_Time InDeltaTime, float InBodyPlanarSpeed, const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs, TArrayView<FProceduralGaitLegOutput> OutOutputs,
            bool InAirborne) -> bool;
        auto UpdatePatternSelection(FCk_Time InDeltaTime, float InBodyPlanarSpeed,
            TArrayView<const FProceduralGaitLegInput> InInputs, bool InAdvance) -> void;
        auto DoSelectPattern(float InBodyPlanarSpeed, bool InAdvance) -> void;
        static auto DoReconcileEnabled(FProceduralGaitLegState& InOutState, const FProceduralGaitLegInput& InInput) -> bool;
        auto DoRedistributeOffsets() -> void;
        auto DoClearRedistribution() -> void;
        static auto DoBeginSwing(FProceduralGaitLegState& InOutState, const FVector& InStartPosition,
            const FQuat& InStartRotation, const FVector& InTarget) -> void;
        static auto DoWriteDisabledOutput(const FProceduralGaitLegState& InState, FProceduralGaitLegOutput& OutOutput) -> void;

    private:
        FProceduralGaitSettings _Settings;
        TArray<FProceduralGaitLegState> _LegStates;
        float _GaitClock = 0.0f;
        float _LastCadenceScale = 1.0f;
        FCk_Time _RestTime;
        bool _WasAirborne = false;
        FProceduralGaitPatternBlendState _PatternBlend;

    public:
        CK_PROPERTY(_Settings);
        CK_PROPERTY_GET(_LastCadenceScale);
    };
}

// --------------------------------------------------------------------------------------------------------------------
