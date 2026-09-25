#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkProceduralAnimation/Core/CkProceduralGaitSwingProfile.h"

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

        float _MinSpeed = 0.f;
        TArray<float, TInlineAllocator<8>> _PhaseOffsets;
        float _CycleDurationScale = 1.f;

    public:
        CK_PROPERTY(_MinSpeed);
        CK_PROPERTY(_PhaseOffsets);
        CK_PROPERTY(_CycleDurationScale);
    };

    struct CKPROCEDURALANIMATION_API FProceduralGaitSettings
    {
        CK_GENERATED_BODY(FProceduralGaitSettings);

    private:
        friend class FProceduralGaitSolver;

        float _StepThreshold = 25.f;
        float _EmergencyStepFactor = 1.75f;
        FCk_Time _StepDuration = FCk_Time{0.25};
        float _StepHeight = 15.f;
        FCk_Time _CycleDuration = FCk_Time{0.5};
        float _SwingWindow = 0.5f;
        float _MoveSpeedThreshold = 5.f;
        float _CadenceSpeedRef = 0.f;
        float _MaxCadenceScale = 3.f;
        float _RetargetSmoothing = 14.f;
        float _RetargetFreezePhase = 0.7f;
        int32 _MaxSimultaneousSwings = 0;
        float _SwingApexPhase = 0.5f;
        float _SwingApexSharpness = 1.f;
        float _SprintApexHeightScale = 0.65f;
        float _ObstacleClearance = 6.f;
        FProceduralGaitSwingProfile _SwingProfile;
        float _StrokeOvershootFraction = 0.25f;
        float _MaxStrokeOvershoot = 20.f;
        float _ScheduleAdvanceFraction = 0.6f;
        float _ScheduleAdvanceRate = 1.5f;
        FCk_Time _CatchStepLifetime = FCk_Time{0.};
        bool _SettleAtRest = true;
        FCk_Time _SettleDelay = FCk_Time{0.35};
        float _SettleThresholdFraction = 0.35f;
        float _AirborneTuckLift = 15.f;
        float _AirborneFollowSpeed = 8.f;
        float _LandingStepDurationScale = 0.5f;
        TArray<FProceduralGaitPattern> _Patterns;
        FCk_Time _PatternBlendTime = FCk_Time{0.4};
        float _PatternSwitchHysteresis = 0.85f;
        float _SwingToePitchDegrees = 0.f;

    public:
        CK_PROPERTY(_StepThreshold);
        CK_PROPERTY(_EmergencyStepFactor);
        CK_PROPERTY(_StepDuration);
        CK_PROPERTY(_StepHeight);
        CK_PROPERTY(_CycleDuration);
        CK_PROPERTY(_SwingWindow);
        CK_PROPERTY(_MoveSpeedThreshold);
        CK_PROPERTY(_CadenceSpeedRef);
        CK_PROPERTY(_MaxCadenceScale);
        CK_PROPERTY(_RetargetSmoothing);
        CK_PROPERTY(_RetargetFreezePhase);
        CK_PROPERTY(_MaxSimultaneousSwings);
        CK_PROPERTY(_SwingApexPhase);
        CK_PROPERTY(_SwingApexSharpness);
        CK_PROPERTY(_SprintApexHeightScale);
        CK_PROPERTY(_ObstacleClearance);
        CK_PROPERTY(_SwingProfile);
        CK_PROPERTY(_StrokeOvershootFraction);
        CK_PROPERTY(_MaxStrokeOvershoot);
        CK_PROPERTY(_ScheduleAdvanceFraction);
        CK_PROPERTY(_ScheduleAdvanceRate);
        CK_PROPERTY(_CatchStepLifetime);
        CK_PROPERTY(_SettleAtRest);
        CK_PROPERTY(_SettleDelay);
        CK_PROPERTY(_SettleThresholdFraction);
        CK_PROPERTY(_AirborneTuckLift);
        CK_PROPERTY(_AirborneFollowSpeed);
        CK_PROPERTY(_LandingStepDurationScale);
        CK_PROPERTY(_Patterns);
        CK_PROPERTY(_PatternBlendTime);
        CK_PROPERTY(_PatternSwitchHysteresis);
        CK_PROPERTY(_SwingToePitchDegrees);
    };

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegInput
    {
        CK_GENERATED_BODY(FProceduralGaitLegInput);

    private:
        friend class FProceduralGaitSolver;

        FVector _IdealTarget = FVector::ZeroVector;
        FVector _GroundNormal = FVector::UpVector;
        float _PhaseOffset = 0.f;
        float _StepThresholdScale = 1.f;
        FVector _FacingDirection = FVector::ForwardVector;
        bool _TargetValid = true;
        float _ClearanceGroundZ = -FLT_MAX;

    public:
        CK_PROPERTY(_IdealTarget);
        CK_PROPERTY(_GroundNormal);
        CK_PROPERTY(_PhaseOffset);
        CK_PROPERTY(_StepThresholdScale);
        CK_PROPERTY(_FacingDirection);
        CK_PROPERTY(_TargetValid);
        CK_PROPERTY(_ClearanceGroundZ);
    };

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegOutput
    {
        CK_GENERATED_BODY(FProceduralGaitLegOutput);

    private:
        friend class FProceduralGaitSolver;

        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        FQuat _Rotation = FQuat::Identity;
        float _SwingAlpha = 0.f;
        bool _Planted = true;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Rotation);
        CK_PROPERTY(_SwingAlpha);
        CK_PROPERTY(_Planted);
    };

    struct CKPROCEDURALANIMATION_API FProceduralGaitLegState
    {
        CK_GENERATED_BODY(FProceduralGaitLegState);

    private:
        friend class FProceduralGaitSolver;

        FVector _PlantedPosition = FVector::ZeroVector;
        FVector _PlantedNormal = FVector::UpVector;
        FVector _SwingStartPosition = FVector::ZeroVector;
        FVector _SwingTarget = FVector::ZeroVector;
        FQuat _PlantedRotation = FQuat::Identity;
        FQuat _SwingStartRotation = FQuat::Identity;
        FVector _AirPosition = FVector::ZeroVector;
        // Transitions start from the pose that was emitted, including clearance and authored swing shape.
        FVector _CurrentPosition = FVector::ZeroVector;
        FQuat _CurrentRotation = FQuat::Identity;
        float _SwingPhase = 0.f;
        float _SwingDurationScale = 1.f;
        bool _Swinging = false;
        bool _TargetFrozen = false;
        bool _Overshoot = false;
        FVector _PendingStepTarget = FVector::ZeroVector;
        FCk_Time _PendingStepTime = FCk_Time{0.};
        bool _CatchStep = false;

    public:
        CK_PROPERTY(_PlantedPosition);
        CK_PROPERTY(_PlantedNormal);
        CK_PROPERTY(_SwingStartPosition);
        CK_PROPERTY(_SwingTarget);
        CK_PROPERTY(_PlantedRotation);
        CK_PROPERTY(_SwingStartRotation);
        CK_PROPERTY(_AirPosition);
        CK_PROPERTY_GET(_CurrentPosition);
        CK_PROPERTY_GET(_CurrentRotation);
        CK_PROPERTY(_SwingPhase);
        CK_PROPERTY(_SwingDurationScale);
        CK_PROPERTY(_Swinging);
        CK_PROPERTY(_TargetFrozen);
        CK_PROPERTY(_Overshoot);
        CK_PROPERTY(_PendingStepTarget);
        CK_PROPERTY(_PendingStepTime);
        CK_PROPERTY(_CatchStep);
    };

    class CKPROCEDURALANIMATION_API FProceduralGaitVelocityTracker
    {
        CK_GENERATED_BODY(FProceduralGaitVelocityTracker);
    public:
        auto Reset() -> void;
        auto Update(const FVector& InWorldPosition, FCk_Time InDeltaTime) -> FVector;
        auto GetVelocity() const -> FVector;
    private:
        static constexpr int32 MaxSamples = 5;
        TArray<FVector, TInlineAllocator<MaxSamples>> _Samples;
        FVector _PreviousPosition = FVector::ZeroVector;
        int32 _NextSample = 0;
        bool _HasPrevious = false;
    };

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
        auto GetGaitClock() const -> float { return _GaitClock; }
        auto IsWindowOpen(float InPhaseOffset) const -> bool;
        auto GetLegState(int32 InLegIndex) const -> const FProceduralGaitLegState& { return _LegStates[InLegIndex]; }
        auto GetEffectivePhaseOffset(int32 InLegIndex) const -> float
        {
            return _EffectivePhaseOffsets.IsValidIndex(InLegIndex) ? _EffectivePhaseOffsets[InLegIndex] : 0.0f;
        }
        auto GetCurrentPatternIndex() const -> int32 { return _CurrentPatternIndex; }
        auto IsAirborne() const -> bool { return _WasAirborne; }
        auto GetRestTime() const -> FCk_Time { return _RestTime; }
        auto SetPlantedPose(int32 InLegIndex, const FVector& InPosition, const FVector& InNormal) -> void;
        auto RequestStep(int32 InLegIndex, const FVector& InTarget) -> bool;
        auto HasPendingStep(int32 InLegIndex) const -> bool
        {
            return _LegStates.IsValidIndex(InLegIndex) && _LegStates[InLegIndex]._PendingStepTime > FCk_Time{};
        }
        auto ClearPendingStep(int32 InLegIndex) -> void
        {
            if (_LegStates.IsValidIndex(InLegIndex))
            { _LegStates[InLegIndex]._PendingStepTime = FCk_Time{}; }
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
        FProceduralGaitSettings _Settings;
        TArray<FProceduralGaitLegState> _LegStates;
        float _GaitClock = 0.0f;
        float _LastCadenceScale = 1.0f;
        FCk_Time _RestTime;
        bool _WasAirborne = false;
        int32 _CurrentPatternIndex = INDEX_NONE;
        float _PatternBlendAlpha = 1.0f;
        float _EffectiveCycleScale = 1.0f;
        float _BlendFromCycleScale = 1.0f;
        TArray<float, TInlineAllocator<8>> _EffectivePhaseOffsets;
        TArray<float, TInlineAllocator<8>> _BlendFromOffsets;
    public:
        CK_PROPERTY(_Settings);
        CK_PROPERTY_GET(_LastCadenceScale);
    };
}
