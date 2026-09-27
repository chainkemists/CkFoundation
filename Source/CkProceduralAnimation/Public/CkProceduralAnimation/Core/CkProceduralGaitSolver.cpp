#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::procedural_gait_solver
{
    auto
        Damp(
            const FVector& InCurrent,
            const FVector& InTarget,
            float InRate,
            FCk_Time InDeltaTime)
        -> FVector
    {
        const auto Alpha = 1.0 - FMath::Exp(-InRate * InDeltaTime.Get_Seconds());
        return FMath::Lerp(InCurrent, InTarget, Alpha);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Damp(
            const FQuat& InCurrent,
            const FQuat& InTarget,
            float InRate,
            FCk_Time InDeltaTime)
        -> FQuat
    {
        const auto Alpha = 1.0 - FMath::Exp(-InRate * InDeltaTime.Get_Seconds());
        return FQuat::Slerp(InCurrent, InTarget, Alpha).GetNormalized();
    }

    // --------------------------------------------------------------------------------------------------------------------

    // What the probe under the landing point last found decides a touchdown's trust; without a probe (a face target, a catch
    // step) the trust the swing recorded for its target does.
    auto
        Get_PlantTrusted(
            const FProceduralGaitLegInput& InInput,
            bool InSwingTargetTrusted)
        -> bool
    {
        switch (InInput.Get_LandingGround())
        {
            case EProceduralGaitLandingGround::Found: return true;
            case EProceduralGaitLandingGround::None: return false;
            case EProceduralGaitLandingGround::Unknown: return InSwingTargetTrusted;
        }
        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProceduralGaitVelocityTracker::
        Update(
            const FVector& InWorldPosition,
            FCk_Time InDeltaTime)
        -> FVector
    {
        if (InWorldPosition.ContainsNaN() || NOT FMath::IsFinite(InDeltaTime.Get_Seconds())
            || InDeltaTime < FCk_Time{})
        { return GetVelocity(); }
        if (NOT _HasPrevious || InDeltaTime <= FCk_Time{KINDA_SMALL_NUMBER})
        {
            _PreviousPosition = InWorldPosition;
            _HasPrevious = true;
            return GetVelocity();
        }

        const auto Instantaneous = (InWorldPosition - _PreviousPosition) / InDeltaTime.Get_Seconds();
        _PreviousPosition = InWorldPosition;

        if (_Samples.Num() < MaxSamples)
        {
            _Samples.Add(Instantaneous);
        }
        else
        {
            _Samples[_NextSample] = Instantaneous;
            _NextSample = (_NextSample + 1) % MaxSamples;
        }

        return GetVelocity();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitVelocityTracker::
        GetVelocity() const
        -> FVector
    {
        if (_Samples.Num() == 0)
        {
            return FVector::ZeroVector;
        }

        auto Sum = FVector::ZeroVector;
        for (const auto& Sample : _Samples)
        {
            Sum += Sample;
        }
        return Sum / static_cast<float>(_Samples.Num());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitYawRateTracker::
        Update(
            const FQuat& InPreviousBasis,
            const FQuat& InBasis,
            FCk_Time InDeltaTime)
        -> float
    {
        if (InPreviousBasis.ContainsNaN() || InBasis.ContainsNaN() || NOT FMath::IsFinite(InDeltaTime.Get_Seconds())
            || InDeltaTime <= FCk_Time{})
        { return _YawRate; }

        const auto LocalDelta = (InPreviousBasis.Inverse() * InBasis).GetNormalized();
        const auto Rate = LocalDelta.GetTwistAngle(FVector::UpVector) / InDeltaTime.Get_Seconds();
        const auto Blend = 1.0 - FMath::Exp(-InDeltaTime.Get_Seconds() / SmoothingTime.Get_Seconds());
        _YawRate = static_cast<float>(FMath::Lerp(static_cast<double>(_YawRate), Rate, Blend));
        return _YawRate;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        Reset(
            TArrayView<const FVector> InInitialFootPositions,
            TArrayView<const bool> InInitialFootTrusted)
        -> bool
    {
        for (const auto& Position : InInitialFootPositions)
        {
            if (Position.ContainsNaN())
            { return false; }
        }

        const auto TrustCoversEveryFoot = InInitialFootTrusted.IsEmpty() || InInitialFootTrusted.Num() == InInitialFootPositions.Num();
        if (NOT TrustCoversEveryFoot)
        { return false; }

        _LegStates.Reset(InInitialFootPositions.Num());
        for (auto Index = 0; Index < InInitialFootPositions.Num(); ++Index)
        {
            const auto& Position = InInitialFootPositions[Index];
            auto& State = _LegStates.AddDefaulted_GetRef();
            State._Plant._Position = Position;
            State._Plant._Trusted = InInitialFootTrusted.IsEmpty() || InInitialFootTrusted[Index];
            State._Emitted._AirPosition = Position;
            State._Emitted._Position = Position;
            State._Emitted._Rotation = State._Plant._Rotation;
        }
        _GaitClock = 0.0f;
        _LastCadenceScale = 1.0f;
        _MissedLandingLifts = 0;
        _RestTime = FCk_Time{};
        _WasAirborne = false;
        _PatternBlend._CurrentIndex = INDEX_NONE;
        _PatternBlend._Alpha = 1.0f;
        _PatternBlend._EffectiveCycleScale = 1.0f;
        _PatternBlend._FromCycleScale = 1.0f;
        _PatternBlend._EffectiveOffsets.Reset();
        _PatternBlend._FromOffsets.Reset();
        _PatternBlend._RedistributedOffsets.Reset();
        _PatternBlend._HasRedistributedOffsets = false;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        IsLegEnabled(
            int32 InLegIndex) const
        -> bool
    {
        return _LegStates.IsValidIndex(InLegIndex) && _LegStates[InLegIndex]._Enabled;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        NumEnabledLegs() const
        -> int32
    {
        return algo::CountIf(_LegStates, [](const FProceduralGaitLegState& InState) -> bool
        {
            return InState.Get_Enabled();
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        MakeFootRotation(
            const FVector& InFacingDirection,
            const FVector& InGroundNormal)
        -> FQuat
    {
        const auto Z = InGroundNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
        auto X = InFacingDirection.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);

        if (FMath::Abs(FVector::DotProduct(X, Z)) > 1.0f - KINDA_SMALL_NUMBER)
        {
            const auto Reference = FMath::Abs(Z.Y) < 0.9f ? FVector::RightVector : FVector::ForwardVector;
            X = FVector::CrossProduct(Z, Reference);
        }
        return FRotationMatrix::MakeFromZX(Z, X).ToQuat();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        ComputeTraceAxis(
            const FVector& InBodyUp,
            const FVector& InRadialDirection,
            float InOutwardLean)
        -> FVector
    {
        const auto Up = InBodyUp.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
        const auto Lean = FMath::Clamp(InOutwardLean, 0.0f, 1.0f);
        if (Lean <= KINDA_SMALL_NUMBER)
        {
            return Up;
        }

        const auto Radial = FVector::VectorPlaneProject(InRadialDirection, Up).GetSafeNormal();
        if (Radial.IsNearlyZero())
        {
            return Up;
        }

        return (Up - Radial * Lean).GetSafeNormal(KINDA_SMALL_NUMBER, Up);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        SetPlantedPose(
            int32 InLegIndex,
            const FVector& InPosition,
            const FVector& InNormal,
            bool InTrusted)
        -> void
    {
        if (InPosition.ContainsNaN() || InNormal.ContainsNaN() || NOT InNormal.IsNormalized())
        { return; }
        if (IsLegEnabled(InLegIndex) && NOT _LegStates[InLegIndex]._Swing._Active)
        {
            auto& State = _LegStates[InLegIndex];

            const auto NormalDelta = FQuat::FindBetweenNormals(
                State._Plant._Normal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector),
                InNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector));
            State._Plant._Rotation = NormalDelta * State._Plant._Rotation;
            State._Plant._Position = InPosition;
            State._Plant._Normal = InNormal;
            State._Plant._Trusted = InTrusted;
            State._Emitted._Position = State._Plant._Position;
            State._Emitted._Rotation = State._Plant._Rotation;
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        SetDisabledPose(
            int32 InLegIndex,
            const FVector& InPosition,
            const FQuat& InRotation,
            const FVector& InNormal)
        -> bool
    {
        const auto LegIsDisabled = _LegStates.IsValidIndex(InLegIndex) && NOT _LegStates[InLegIndex]._Enabled;
        const auto PoseIsValid = NOT InPosition.ContainsNaN()
            && NOT InRotation.ContainsNaN() && InRotation.IsNormalized()
            && NOT InNormal.ContainsNaN() && InNormal.IsNormalized();

        if (NOT LegIsDisabled || NOT PoseIsValid)
        { return false; }

        auto& State = _LegStates[InLegIndex];
        State._Emitted._Position = InPosition;
        State._Emitted._Rotation = InRotation;
        State._Plant._Position = InPosition;
        State._Plant._Rotation = InRotation;
        State._Plant._Normal = InNormal;
        State._Plant._Trusted = false;
        State._Emitted._AirPosition = InPosition;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        RequestStep(
            int32 InLegIndex,
            const FVector& InTarget)
        -> bool
    {
        if (NOT IsLegEnabled(InLegIndex) || InTarget.ContainsNaN() || NOT ValidateSettings(_Settings))
        {
            return false;
        }

        auto& State = _LegStates[InLegIndex];
        State._PendingStep._Target = InTarget;

        State._PendingStep._Time = _Settings._Schedule._CatchStepLifetime > FCk_Time{}
            ? _Settings._Schedule._CatchStepLifetime
            : FMath::Max(_Settings._Cadence._CycleDuration, FCk_Time{KINDA_SMALL_NUMBER});
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        TransformState(
            const FQuat& InDelta)
        -> void
    {
        if (InDelta.ContainsNaN() || NOT InDelta.IsNormalized())
        { return; }
        for (auto& State : _LegStates)
        {
            State._PendingStep._Target = InDelta.RotateVector(State._PendingStep._Target);
            State._Plant._Position = InDelta.RotateVector(State._Plant._Position);
            State._Plant._Normal = InDelta.RotateVector(State._Plant._Normal);
            State._Swing._StartPosition = InDelta.RotateVector(State._Swing._StartPosition);
            State._Swing._Target = InDelta.RotateVector(State._Swing._Target);
            State._Swing._LandingPoint = InDelta.RotateVector(State._Swing._LandingPoint);
            State._Swing._LiftedLandingPoint = InDelta.RotateVector(State._Swing._LiftedLandingPoint);
            State._Swing._ValidatedTarget = InDelta.RotateVector(State._Swing._ValidatedTarget);
            State._Swing._PullBackFrom = InDelta.RotateVector(State._Swing._PullBackFrom);
            State._Swing._TargetNormal = InDelta.RotateVector(State._Swing._TargetNormal);
            State._Emitted._AirPosition = InDelta.RotateVector(State._Emitted._AirPosition);
            State._Emitted._Position = InDelta.RotateVector(State._Emitted._Position);
            State._Emitted._Rotation = InDelta * State._Emitted._Rotation;
            State._Plant._Rotation = InDelta * State._Plant._Rotation;
            State._Swing._StartRotation = InDelta * State._Swing._StartRotation;
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        IsWindowOpen(
            float InPhaseOffset) const
        -> bool
    {
        const auto Local = FMath::Frac(_GaitClock - InPhaseOffset + 1.0f);
        return Local < _Settings._Cadence._SwingWindow;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        UpdatePatternSelection(
            FCk_Time InDeltaTime,
            float InBodyPlanarSpeed,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            bool InAdvance)
        -> void
    {
        const auto NumInputs = InInputs.Num();
        _PatternBlend._EffectiveOffsets.SetNum(NumInputs);

        const auto HasPatterns = _Settings._Pattern._Patterns.Num() > 0;
        if (NOT HasPatterns && NOT _PatternBlend._HasRedistributedOffsets && _PatternBlend._Alpha >= 1.0f)
        {
            for (auto LegIndex = 0; LegIndex < NumInputs; ++LegIndex)
            {
                _PatternBlend._EffectiveOffsets[LegIndex] = InInputs[LegIndex]._PhaseOffset;
            }
            _PatternBlend._CurrentIndex = INDEX_NONE;
            _PatternBlend._Alpha = 1.0f;
            _PatternBlend._EffectiveCycleScale = 1.0f;
            return;
        }

        if (HasPatterns)
        { DoSelectPattern(InBodyPlanarSpeed, InAdvance); }
        else
        { _PatternBlend._CurrentIndex = INDEX_NONE; }

        if (InAdvance)
        {
            _PatternBlend._Alpha = FMath::Min(1.0f,
                _PatternBlend._Alpha + static_cast<float>(InDeltaTime / FMath::Max(_Settings._Pattern._BlendTime, FCk_Time{KINDA_SMALL_NUMBER})));
        }

        const auto Pattern = HasPatterns ? &_Settings._Pattern._Patterns[_PatternBlend._CurrentIndex] : nullptr;
        const auto Alpha = FMath::SmoothStep(0.0f, 1.0f, _PatternBlend._Alpha);
        for (auto LegIndex = 0; LegIndex < NumInputs; ++LegIndex)
        {
            if (_PatternBlend._HasRedistributedOffsets && NOT _LegStates[LegIndex]._Enabled)
            { continue; }

            const auto AuthoredOffset = HasPatterns && Pattern->_PhaseOffsets.IsValidIndex(LegIndex)
                ? Pattern->_PhaseOffsets[LegIndex]
                : InInputs[LegIndex]._PhaseOffset;
            const auto Target = _PatternBlend._HasRedistributedOffsets ? _PatternBlend._RedistributedOffsets[LegIndex] : AuthoredOffset;
            const auto From = _PatternBlend._FromOffsets.IsValidIndex(LegIndex) ? _PatternBlend._FromOffsets[LegIndex] : Target;

            const auto Delta = FMath::Frac(Target - From + 1.5f) - 0.5f;
            _PatternBlend._EffectiveOffsets[LegIndex] = FMath::Frac(From + Delta * Alpha + 1.0f);
        }
        _PatternBlend._EffectiveCycleScale = HasPatterns
            ? FMath::Lerp(_PatternBlend._FromCycleScale, Pattern->_CycleDurationScale, Alpha)
            : 1.0f;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoSelectPattern(
            float InBodyPlanarSpeed,
            bool InAdvance)
        -> void
    {
        auto Desired = int32{INDEX_NONE};
        auto DesiredMinSpeed = -FLT_MAX;
        auto Slowest = 0;
        auto SlowestMinSpeed = FLT_MAX;
        for (auto PatternIndex = 0; PatternIndex < _Settings._Pattern._Patterns.Num(); ++PatternIndex)
        {
            const auto MinSpeed = _Settings._Pattern._Patterns[PatternIndex]._MinSpeed;
            if (MinSpeed < SlowestMinSpeed)
            {
                SlowestMinSpeed = MinSpeed;
                Slowest = PatternIndex;
            }
            if (MinSpeed <= InBodyPlanarSpeed && MinSpeed > DesiredMinSpeed)
            {
                DesiredMinSpeed = MinSpeed;
                Desired = PatternIndex;
            }
        }
        if (Desired == INDEX_NONE)
        {
            Desired = Slowest;
        }

        if (NOT _Settings._Pattern._Patterns.IsValidIndex(_PatternBlend._CurrentIndex))
        {
            _PatternBlend._CurrentIndex = Desired;
            _PatternBlend._Alpha = 1.0f;
            _PatternBlend._FromOffsets.Reset();
            _PatternBlend._FromCycleScale = _Settings._Pattern._Patterns[Desired]._CycleDurationScale;
        }
        else if (Desired != _PatternBlend._CurrentIndex && InAdvance)
        {
            const auto Downward =
                _Settings._Pattern._Patterns[Desired]._MinSpeed < _Settings._Pattern._Patterns[_PatternBlend._CurrentIndex]._MinSpeed;
            const auto Allowed = NOT Downward
                || InBodyPlanarSpeed < _Settings._Pattern._Patterns[_PatternBlend._CurrentIndex]._MinSpeed
                    * FMath::Clamp(_Settings._Pattern._SwitchHysteresis, 0.0f, 1.0f);
            if (Allowed)
            {
                _PatternBlend._FromOffsets = _PatternBlend._EffectiveOffsets;
                _PatternBlend._FromCycleScale = _PatternBlend._EffectiveCycleScale;
                _PatternBlend._Alpha = 0.0f;
                _PatternBlend._CurrentIndex = Desired;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        ValidateSettings(
            const FProceduralGaitSettings& InSettings)
        -> bool
    {
        const float ScalarValues[] =
        {
            InSettings._Step._Threshold,
            InSettings._Step._EmergencyFactor,
            InSettings._Swing._Height,
            InSettings._Cadence._SwingWindow,
            InSettings._Cadence._MoveSpeedThreshold,
            InSettings._Cadence._CadenceSpeedRef,
            InSettings._Cadence._MaxCadenceScale,
            InSettings._Step._RetargetSmoothing,
            InSettings._Step._RetargetFreezePhase,
            InSettings._Swing._ApexPhase,
            InSettings._Swing._ApexSharpness,
            InSettings._Swing._SprintApexHeightScale,
            InSettings._Swing._ObstacleClearance,
            InSettings._Step._StrokeOvershootFraction,
            InSettings._Step._MaxStrokeOvershoot,
            InSettings._Schedule._AdvanceFraction,
            InSettings._Schedule._AdvanceRate,
            InSettings._Settle._ThresholdFraction,
            InSettings._Airborne._TuckLift,
            InSettings._Airborne._FollowSpeed,
            InSettings._Airborne._LandingStepDurationScale,
            InSettings._Pattern._SwitchHysteresis,
            InSettings._Swing._ToePitchDegrees,
            InSettings._Reach._TargetFraction,
            InSettings._Reach._ForceStepFraction,
            InSettings._Reach._HardOverstretchFraction,
            InSettings._Reach._TouchdownLiftFraction,
        };
        for (const auto Value : ScalarValues)
        {
            if (NOT FMath::IsFinite(Value))
            { return false; }
        }
        const FCk_Time Durations[] =
        {
            InSettings._Step._Duration,
            InSettings._Cadence._CycleDuration,
            InSettings._Schedule._CatchStepLifetime,
            InSettings._Settle._Delay,
            InSettings._Pattern._BlendTime,
        };
        for (const auto Duration : Durations)
        {
            if (NOT FMath::IsFinite(Duration.Get_Seconds()) || Duration < FCk_Time{})
            { return false; }
        }
        const auto UnitInterval = [](float InValue) -> bool { return InValue >= 0.0f && InValue <= 1.0f; };
        const auto PositiveUnitInterval = [](float InValue) -> bool { return InValue > 0.0f && InValue <= 1.0f; };
        if (InSettings._Step._Threshold <= 0.0f || InSettings._Step._Duration <= FCk_Time{}
            || InSettings._Cadence._CycleDuration <= FCk_Time{} || InSettings._Swing._Height < 0.0f
            || InSettings._Step._EmergencyFactor < 1.0f || NOT PositiveUnitInterval(InSettings._Cadence._SwingWindow)
            || InSettings._Cadence._MoveSpeedThreshold < 0.0f || InSettings._Cadence._CadenceSpeedRef < 0.0f
            || InSettings._Cadence._MaxCadenceScale < 1.0f || InSettings._Step._RetargetSmoothing < 0.0f
            || NOT UnitInterval(InSettings._Step._RetargetFreezePhase) || InSettings._Cadence._MaxSimultaneousSwings < 0
            || NOT UnitInterval(InSettings._Swing._ApexPhase) || InSettings._Swing._ApexSharpness <= 0.0f
            || InSettings._Swing._SprintApexHeightScale < 0.0f || InSettings._Swing._ObstacleClearance < 0.0f
            || InSettings._Step._StrokeOvershootFraction < 0.0f || InSettings._Step._MaxStrokeOvershoot < 0.0f
            || NOT UnitInterval(InSettings._Schedule._AdvanceFraction) || InSettings._Schedule._AdvanceRate < 0.0f
            || NOT PositiveUnitInterval(InSettings._Settle._ThresholdFraction) || InSettings._Airborne._TuckLift < 0.0f
            || InSettings._Airborne._FollowSpeed < 0.0f || NOT PositiveUnitInterval(InSettings._Airborne._LandingStepDurationScale)
            || NOT PositiveUnitInterval(InSettings._Pattern._SwitchHysteresis)
            || InSettings._Reach._TargetFraction <= 0.0f || InSettings._Reach._TargetFraction >= InSettings._Reach._ForceStepFraction
            || InSettings._Reach._ForceStepFraction > 1.0f
            || InSettings._Reach._HardOverstretchFraction < 1.0f || InSettings._Reach._HardOverstretchFraction > 1.5f
            || InSettings._Reach._HardOverstretchFraction < InSettings._Reach._ForceStepFraction
            || NOT UnitInterval(InSettings._Reach._TouchdownLiftFraction))
        { return false; }
        for (const auto& Pattern : InSettings._Pattern._Patterns)
        {
            if (NOT FMath::IsFinite(Pattern._MinSpeed) || Pattern._MinSpeed < 0.0f
                || NOT FMath::IsFinite(Pattern._CycleDurationScale) || Pattern._CycleDurationScale <= 0.0f)
            { return false; }
            for (const auto Offset : Pattern._PhaseOffsets)
            {
                if (NOT FMath::IsFinite(Offset) || Offset < 0.0f || Offset >= 1.0f)
                { return false; }
            }
        }
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        Step(
            FCk_Time InDeltaTime,
            float InBodyPlanarSpeed,
            const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            TArrayView<FProceduralGaitLegOutput> OutOutputs,
            bool InAirborne)
        -> bool
    {
        if (InInputs.Num() != OutOutputs.Num() || InInputs.Num() != _LegStates.Num()
            || NOT FMath::IsFinite(InDeltaTime.Get_Seconds()) || NOT FMath::IsFinite(InBodyPlanarSpeed)
            || InBodyPlanarSpeed < 0.0f || InBodyPlanarVelocity.ContainsNaN() || NOT ValidateSettings(_Settings))
        { return false; }
        for (const auto& Input : InInputs)
        {
            if (Input._IdealTarget.ContainsNaN() || Input._GroundNormal.ContainsNaN()
                || Input._FacingDirection.ContainsNaN() || NOT FMath::IsFinite(Input._PhaseOffset)
                || Input._PhaseOffset < 0.0f || Input._PhaseOffset >= 1.0f
                || NOT FMath::IsFinite(Input._StepThresholdScale) || Input._StepThresholdScale <= 0.0f
                || NOT FMath::IsFinite(Input._ClearanceGroundZ) || NOT FMath::IsFinite(Input._LandingGroundZ)
                || (Input._TargetValid && NOT Input._GroundNormal.IsNormalized())
                || Input._Hip.ContainsNaN() || NOT FMath::IsFinite(Input._Reach) || Input._Reach < 0.0f)
            { return false; }
        }

        auto EffectiveInputs = TArray<FProceduralGaitLegInput, TInlineAllocator<16>>{};
        EffectiveInputs.Reserve(InInputs.Num());
        for (const auto& Input : InInputs)
        { EffectiveInputs.Add(DoGet_EffectiveInput(Input)); }

        if (InDeltaTime <= FCk_Time{})
        {
            auto ReadOnlySolver = *this;
            return ReadOnlySolver.DoStep(InDeltaTime, InBodyPlanarSpeed, InBodyPlanarVelocity,
                EffectiveInputs, OutOutputs, InAirborne);
        }
        const auto Advanced = DoStep(InDeltaTime, InBodyPlanarSpeed, InBodyPlanarVelocity, EffectiveInputs, OutOutputs, InAirborne);
        if (Advanced)
        {
            for (auto LegIndex = 0; LegIndex < OutOutputs.Num(); ++LegIndex)
            {
                _LegStates[LegIndex]._Emitted._Position = OutOutputs[LegIndex]._Position;
                _LegStates[LegIndex]._Emitted._Rotation = OutOutputs[LegIndex]._Rotation;
            }
        }
        return Advanced;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoStep(
            FCk_Time InDeltaTime,
            float InBodyPlanarSpeed,
            const FVector& InBodyPlanarVelocity,
            TArrayView<const FProceduralGaitLegInput> InInputs,
            TArrayView<FProceduralGaitLegOutput> OutOutputs,
            bool InAirborne)
        -> bool
    {
        const auto Advance = InDeltaTime > FCk_Time{};

        auto EnabledSetChanged = false;
        for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
        {
            if (DoReconcileEnabled(_LegStates[LegIndex], InInputs[LegIndex]))
            { EnabledSetChanged = true; }
        }

        if (EnabledSetChanged && _Settings._Pattern._LegLossPolicy == EProceduralGaitLegLossPolicy::RedistributeOffsets)
        {
            if (NumEnabledLegs() == NumLegs())
            { DoClearRedistribution(); }
            else
            { DoRedistributeOffsets(); }
        }

        if (_Settings._Pattern._LegLossPolicy == EProceduralGaitLegLossPolicy::KeepAuthoredOffsets && _PatternBlend._HasRedistributedOffsets)
        { DoClearRedistribution(); }

        if (Advance)
        {
            for (auto& State : _LegStates)
            {
                if (State._PendingStep._Time > FCk_Time{})
                {
                    State._PendingStep._Time = FMath::Max(State._PendingStep._Time - InDeltaTime, FCk_Time{});
                }
            }
        }

        UpdatePatternSelection(InDeltaTime, InBodyPlanarSpeed, InInputs, Advance);

        auto CadenceScale = 1.0f;
        if (_Settings._Cadence._CadenceSpeedRef > KINDA_SMALL_NUMBER)
        {
            CadenceScale = FMath::Clamp(InBodyPlanarSpeed / _Settings._Cadence._CadenceSpeedRef, 1.0f, FMath::Max(_Settings._Cadence._MaxCadenceScale, 1.0f));
        }

        if (Advance)
        { _LastCadenceScale = CadenceScale; }

        const auto Moving = InBodyPlanarSpeed > _Settings._Cadence._MoveSpeedThreshold;

        if (Advance)
        {
            _RestTime = (NOT Moving && NOT InAirborne) ? _RestTime + InDeltaTime : FCk_Time{};
        }
        const auto SettleActive = _Settings._Settle._AtRest && _RestTime >= _Settings._Settle._Delay;

        if (Advance && Moving && NOT InAirborne)
        {
            _GaitClock = FMath::Frac(_GaitClock + static_cast<float>(InDeltaTime * CadenceScale
                / FMath::Max(_Settings._Cadence._CycleDuration * _PatternBlend._EffectiveCycleScale, FCk_Time{KINDA_SMALL_NUMBER})));
        }

        if (Advance && InAirborne && NOT _WasAirborne)
        {
            for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
            {
                auto& State = _LegStates[LegIndex];
                if (NOT State._Enabled)
                { continue; }

                State._Emitted._AirPosition = State._Emitted._Position;
                State._Plant._Rotation = State._Emitted._Rotation;
                State._Swing._Active = false;
                State._Swing._Phase = 0.0f;
                State._Swing._TargetFrozen = false;
                State._Swing._Overshoot = false;
                State._Swing._CatchStep = false;
                State._Swing._BeyondSchedule = false;
                State._Swing._DurationScale = 1.0f;
            }
        }
        else if (Advance && NOT InAirborne && _WasAirborne)
        {
            for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
            {
                auto& State = _LegStates[LegIndex];
                if (NOT State._Enabled)
                { continue; }

                const auto& In = InInputs[LegIndex];
                State._Swing._Active = true;
                State._Swing._Phase = 0.0f;
                State._Swing._StartPosition = State._Emitted._AirPosition;
                State._Swing._StartRotation = State._Plant._Rotation;
                State._Swing._Target = DoClampTarget(In, In._TargetValid ? In._IdealTarget : State._Emitted._AirPosition,
                    In._TargetValid && In._TargetTrusted);
                State._Swing._LandingPoint = State._Swing._Target;
                State._Swing._ValidatedTarget = State._Swing._Target;
                State._Swing._TargetTrusted = In._TargetValid && In._TargetTrusted;
                State._Swing._TargetNormal = In._TargetValid ? In._GroundNormal : State._Plant._Normal;
                State._Swing._TargetOnAFace = In._TargetValid && Get_IsFaceNormal(In._GroundNormal, FVector::UpVector);
                State._Swing._LandsOnTarget = false;
                State._Swing._PullBackStartAlpha = -1.0f;
                State._Swing._TargetFrozen = false;
                State._Swing._LandingLiftStartAlpha = -1.0f;

                State._Swing._Overshoot = false;

                State._Swing._CatchStep = false;
                State._Swing._BeyondSchedule = false;
                State._Swing._DurationScale = FMath::Clamp(_Settings._Airborne._LandingStepDurationScale, 0.1f, 1.0f);
            }
        }
        if (Advance)
        {
            _WasAirborne = InAirborne;
        }

        if (InAirborne)
        {
            for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
            {
                const auto& In = InInputs[LegIndex];
                auto& State = _LegStates[LegIndex];
                auto& Out = OutOutputs[LegIndex];

                if (NOT State._Enabled)
                {
                    DoWriteDisabledOutput(State, Out);
                    continue;
                }

                const auto TuckTarget = In._IdealTarget + FVector{0.0, 0.0, _Settings._Airborne._TuckLift};
                const auto TuckRotation = MakeFootRotation(In._FacingDirection, FVector::UpVector);
                if (Advance)
                {
                    State._Emitted._AirPosition = procedural_gait_solver::Damp(State._Emitted._AirPosition, TuckTarget, FMath::Max(_Settings._Airborne._FollowSpeed, KINDA_SMALL_NUMBER), InDeltaTime);
                    State._Plant._Rotation = procedural_gait_solver::Damp(State._Plant._Rotation, TuckRotation,
                        FMath::Max(_Settings._Airborne._FollowSpeed, KINDA_SMALL_NUMBER), InDeltaTime);
                }

                Out._Position = State._Emitted._AirPosition;
                Out._Normal = FVector::UpVector;
                Out._Rotation = State._Plant._Rotation;
                Out._SwingAlpha = 1.0f;
                Out._Planted = false;
            }
            return true;
        }

        // The phase schedule swings one group at a time. Beside it, the group of a hard-overstretched foot may swing beyond the
        // schedule; those swings neither inhibit the schedule's take-offs nor wait for them, and only one group swings there.
        auto NumSwinging = 0;
        auto ScheduledOffsets = TArray<float, TInlineAllocator<8>>{};
        auto BeyondScheduleOffsets = TArray<float, TInlineAllocator<8>>{};
        for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
        {
            const auto& Swing = _LegStates[LegIndex]._Swing;
            if (NOT Swing._Active)
            { continue; }

            ++NumSwinging;
            (Swing._BeyondSchedule ? BeyondScheduleOffsets : ScheduledOffsets).Add(_PatternBlend._EffectiveOffsets[LegIndex]);
        }

        const auto ContainsGroup = [](TArrayView<const float> InOffsets, float InPhaseOffset) -> bool
        {
            return algo::AnyOf(InOffsets, [InPhaseOffset](float InOffset)
            {
                return FMath::IsNearlyEqual(InOffset, InPhaseOffset, 1.0e-3f);
            });
        };

        const auto IsInhibited = [&ScheduledOffsets](float InPhaseOffset) -> bool
        {
            return algo::AnyOf(ScheduledOffsets, [InPhaseOffset](float InOffset)
            {
                return NOT FMath::IsNearlyEqual(InOffset, InPhaseOffset, 1.0e-3f);
            });
        };

        if (Advance && _Settings._Schedule._AdvanceFraction > 0.0f && _Settings._Schedule._AdvanceRate > 0.0f)
        {
            const auto AdvanceTrigger = FMath::Max(_Settings._Step._EmergencyFactor, 1.0f)
                * FMath::Clamp(_Settings._Schedule._AdvanceFraction, 0.0f, 1.0f);

            auto WorstRatio = AdvanceTrigger;
            auto WorstGap = 0.0f;
            for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
            {
                const auto& In = InInputs[LegIndex];
                const auto& State = _LegStates[LegIndex];
                const auto Offset = _PatternBlend._EffectiveOffsets[LegIndex];

                if (NOT State._Enabled || State._Swing._Active || NOT In._TargetValid || IsWindowOpen(Offset))
                {
                    continue;
                }

                const auto Threshold = _Settings._Step._Threshold * FMath::Max(In._StepThresholdScale, KINDA_SMALL_NUMBER);
                const auto Ratio = FVector::Dist(State._Plant._Position, In._IdealTarget) / Threshold;
                if (Ratio > WorstRatio)
                {
                    WorstRatio = Ratio;

                    WorstGap = FMath::Frac(Offset - _GaitClock + 1.0f);
                }
            }

            if (WorstGap > 0.0f)
            {
                _GaitClock = FMath::Frac(_GaitClock
                    + FMath::Min(WorstGap, static_cast<float>(InDeltaTime.Get_Seconds()) * _Settings._Schedule._AdvanceRate) + 1.0f);
            }
        }

        const auto HasSwingInGroup = [&](float InPhaseOffset) -> bool
        {
            return ContainsGroup(ScheduledOffsets, InPhaseOffset) || ContainsGroup(BeyondScheduleOffsets, InPhaseOffset);
        };

        // An Emergency leg waits on inhibition while other groups keep starting swings, and on the frame they drain,
        // leg index order hands the slot to another group again. The Emergency leg whose group has nothing in flight
        // therefore holds back every other group's take-off until it steps, Emergency take-offs included: while the body
        // turns in place every group is in Emergency at once, and index order would starve one of them. Only a
        // hard-overstretched or occluded foot steps past it, beyond the schedule.
        auto PriorityPhaseOffset = TOptional<float>{};
        auto PriorityRatio = 0.0;
        for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
        {
            const auto& In = InInputs[LegIndex];
            const auto& State = _LegStates[LegIndex];
            const auto Offset = _PatternBlend._EffectiveOffsets[LegIndex];

            if (NOT State._Enabled || State._Swing._Active || NOT In._TargetValid || HasSwingInGroup(Offset)
                || NOT DoGet_IsEmergency(State, In))
            { continue; }

            const auto Ratio = DoGet_EmergencyRatio(State, In);
            if (NOT PriorityPhaseOffset.IsSet() || Ratio > PriorityRatio)
            {
                PriorityPhaseOffset = Offset;
                PriorityRatio = Ratio;
            }
        }

        for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
        {
            const auto& In = InInputs[LegIndex];
            auto& State = _LegStates[LegIndex];
            auto& Out = OutOutputs[LegIndex];
            const auto LegPhaseOffset = _PatternBlend._EffectiveOffsets[LegIndex];

            if (NOT State._Enabled)
            {
                DoWriteDisabledOutput(State, Out);
                continue;
            }

            if (State._Swing._Active)
            {
                const auto SwingDuration = FMath::Max(_Settings._Step._Duration * State._Swing._DurationScale / CadenceScale, FCk_Time{KINDA_SMALL_NUMBER});
                const auto PreviousPhase = State._Swing._Phase;
                if (Advance)
                {
                    State._Swing._Phase = FMath::Min(State._Swing._Phase + static_cast<float>(InDeltaTime / SwingDuration), 1.0f);

                    if (NOT State._Swing._CatchStep && NOT State._Swing._TargetFrozen && In._TargetValid
                        && Get_IsFaceNormal(In._GroundNormal, FVector::UpVector))
                    { State._Swing._TargetOnAFace = true; }

                    if (NOT State._Swing._TargetFrozen && In._TargetValid && (In._TargetIsFoothold || State._Swing._TargetOnAFace))
                    { State._Swing._Overshoot = false; }

                    // Nothing under the landing point means the overshoot and the freeze push carry the foot past the ground
                    // the caller validated (off a top's edge, into the air): before the freeze the swing gives both up, after
                    // it the landing is pulled back onto the validated target, for the rest of the swing either way.
                    const auto NoLandingGround = NOT State._Swing._CatchStep && In._LandingGround == EProceduralGaitLandingGround::None;
                    if (NoLandingGround && NOT State._Swing._TargetFrozen)
                    {
                        State._Swing._LandsOnTarget = true;
                        State._Swing._Overshoot = false;
                    }

                    if (NOT State._Swing._CatchStep && NOT State._Swing._TargetFrozen
                        && State._Swing._Phase >= _Settings._Step._RetargetFreezePhase && In._TargetValid)
                    {
                        State._Swing._ValidatedTarget = DoClampTarget(In, In._IdealTarget, In._TargetTrusted);
                        State._Swing._Target = DoGet_FrozenTarget(State, In, InBodyPlanarVelocity,
                            SwingDuration * (1.0f - State._Swing._Phase));
                        State._Swing._TargetTrusted = In._TargetTrusted;
                        State._Swing._TargetNormal = In._GroundNormal;
                        State._Swing._TargetFrozen = true;
                    }
                    else if (NOT State._Swing._CatchStep && NOT State._Swing._TargetFrozen && In._TargetValid)
                    {
                        State._Swing._Target = DoClampTarget(In, procedural_gait_solver::Damp(State._Swing._Target, In._IdealTarget,
                            FMath::Max(_Settings._Step._RetargetSmoothing, KINDA_SMALL_NUMBER), InDeltaTime), In._TargetTrusted);
                        State._Swing._TargetTrusted = In._TargetTrusted;
                        State._Swing._TargetNormal = In._GroundNormal;
                    }
                    else if (NoLandingGround && State._Swing._TargetFrozen && State._Swing._PullBackStartAlpha < 0.0f)
                    {
                        State._Swing._PullBackFrom = DoGet_LandingPoint(State, In, State._Swing._ValidatedTarget, State._Swing._Target);
                        State._Swing._PullBackStartAlpha = DoGet_SwingEase(PreviousPhase);
                    }
                }

                // After the freeze the swing's target carries the push, and the target the freeze took before it is the
                // validated one; before, the target carries no displacement yet.
                const auto UndisplacedTarget = State._Swing._TargetFrozen ? State._Swing._ValidatedTarget : State._Swing._Target;
                auto Target = DoGet_LandingPoint(State, In, UndisplacedTarget, State._Swing._Target);
                if (State._Swing._PullBackStartAlpha >= 0.0f)
                {
                    const auto PullBackStart = State._Swing._PullBackStartAlpha;
                    const auto PullBackShare = PullBackStart < 1.0f - KINDA_SMALL_NUMBER
                        ? FMath::Clamp((DoGet_SwingEase(State._Swing._Phase) - PullBackStart) / (1.0f - PullBackStart), 0.0f, 1.0f)
                        : 1.0f;
                    Target = FMath::Lerp(State._Swing._PullBackFrom, State._Swing._ValidatedTarget, static_cast<double>(PullBackShare));
                }
                // Before the freeze the damped target trails the moving ideal and the freeze snaps it forward, so the point
                // reported for probing is the one the freeze will produce: a swing too short to lift after the snap still
                // learns the ground it lands on in time. The report reaches the solver a frame later, when the ideal has moved
                // on by a frame of travel, so the prediction starts from there.
                const auto PredictsTheFreeze = NOT State._Swing._TargetFrozen && NOT State._Swing._CatchStep && In._TargetValid;
                State._Swing._LandingPoint = PredictsTheFreeze
                    ? DoGet_LandingPoint(State, In, DoClampTarget(In, In._IdealTarget, In._TargetTrusted),
                        DoGet_FrozenTarget(State, In, InBodyPlanarVelocity,
                            SwingDuration * (1.0f - _Settings._Step._RetargetFreezePhase) + InDeltaTime))
                    : Target;

                // The overshoot and the freeze push can carry a target probed on a lower tread past the next riser; the ground
                // under the landing point then lies above it.
                const auto HasLandingGround = NOT State._Swing._CatchStep && In._LandingGround == EProceduralGaitLandingGround::Found;
                const auto OnLandingGround = FVector{Target.X, Target.Y, In._LandingGroundZ};
                const auto LandingGroundAbove = HasLandingGround && In._LandingGroundZ > Target.Z
                    && (In._Reach <= 0.0f || FVector::Dist(OnLandingGround, In._Hip) <= _Settings._Reach._TargetFraction * In._Reach);
                // A lifted swing follows the latest report, down to no lift at all: the landing point can move off the upper
                // tread after the lift began, and a plant must not hover over the lower one. A report other than Found leaves
                // the last lift as it is.
                if (State._Swing._LandingLiftStartAlpha >= 0.0f)
                {
                    if (HasLandingGround)
                    { State._Swing._LiftedLandingPoint = LandingGroundAbove ? OnLandingGround : Target; }
                }
                else if (LandingGroundAbove && State._Swing._Phase < 1.0f)
                {
                    State._Swing._LandingLiftStartAlpha = DoGet_SwingEase(PreviousPhase);
                    State._Swing._LiftedLandingPoint = OnLandingGround;
                }

                const auto LandingRotation = MakeFootRotation(In._FacingDirection, State._Swing._TargetNormal);

                if (State._Swing._Phase >= 1.0f)
                {
                    State._Swing._Active = false;
                    State._Swing._Phase = 0.0f;
                    State._Swing._DurationScale = 1.0f;
                    State._Swing._Overshoot = false;
                    State._Swing._BeyondSchedule = false;

                    if (In._TargetValid && NOT State._Swing._CatchStep)
                    {
                        const auto BelowGround = FVector::DotProduct(In._IdealTarget - Target, In._GroundNormal);
                        if (BelowGround > 0.0f)
                        {
                            Target += In._GroundNormal * BelowGround;
                        }
                    }
                    if (State._Swing._LandingLiftStartAlpha >= 0.0f)
                    { Target.Z = FMath::Max(Target.Z, State._Swing._LiftedLandingPoint.Z); }
                    else if (LandingGroundAbove)
                    {
                        // A report that first arrives at touchdown may still lift the plant this far, below what reads as a
                        // pop; anything more is left and counted.
                        const auto MaxTouchdownLift = _Settings._Reach._TouchdownLiftFraction * _Settings._Swing._Height;
                        if (In._LandingGroundZ - Target.Z <= MaxTouchdownLift)
                        { Target.Z = In._LandingGroundZ; }
                        else
                        { ++_MissedLandingLifts; }
                    }
                    State._Swing._LandingLiftStartAlpha = -1.0f;
                    State._Swing._CatchStep = false;

                    State._Plant._Position = Target;
                    State._Plant._Normal = State._Swing._TargetNormal;
                    State._Plant._Rotation = LandingRotation;
                    State._Plant._Trusted = procedural_gait_solver::Get_PlantTrusted(In, State._Swing._TargetTrusted);

                    Out._Position = State._Plant._Position;
                    Out._Normal = State._Plant._Normal;
                    Out._Rotation = State._Plant._Rotation;
                    Out._SwingAlpha = 0.0f;
                    Out._Planted = true;
                }
                else
                {
                    const auto Alpha = DoGet_SwingEase(State._Swing._Phase);
                    const auto RotationAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
                    auto Position = FMath::Lerp(State._Swing._StartPosition, Target, Alpha);
                    if (State._Swing._LandingLiftStartAlpha >= 0.0f)
                    {
                        const auto LiftStart = State._Swing._LandingLiftStartAlpha;
                        const auto LiftShare = LiftStart < 1.0f - KINDA_SMALL_NUMBER
                            ? FMath::Clamp((Alpha - LiftStart) / (1.0f - LiftStart), 0.0f, 1.0f)
                            : 1.0f;
                        Position.Z += FMath::Max(State._Swing._LiftedLandingPoint.Z - Target.Z, 0.0) * LiftShare;
                    }

                    const auto ArcAlpha = _Settings._Swing._Profile.IsArcValid()
                        ? _Settings._Swing._Profile.SampleArc(State._Swing._Phase)
                        : procedural_gait_swing::ParametricArc(
                            State._Swing._Phase, _Settings._Swing._ApexPhase, _Settings._Swing._ApexSharpness);

                    auto HeightScale = 1.0f;
                    if (_Settings._Cadence._MaxCadenceScale > 1.0f + KINDA_SMALL_NUMBER)
                    {
                        const auto SprintBlend = (CadenceScale - 1.0f) / (_Settings._Cadence._MaxCadenceScale - 1.0f);
                        HeightScale = FMath::Lerp(1.0f, _Settings._Swing._SprintApexHeightScale, FMath::Clamp(SprintBlend, 0.0f, 1.0f));
                    }
                    Position.Z += ArcAlpha * _Settings._Swing._Height * HeightScale;

                    if (_Settings._Swing._ObstacleClearance > 0.0f && In._ClearanceGroundZ > -FLT_MAX * 0.5f)
                    {
                        const auto Deficit = (In._ClearanceGroundZ + _Settings._Swing._ObstacleClearance) - Position.Z;
                        if (Deficit > 0.0f)
                        {
                            Position.Z += Deficit * FMath::Sin(State._Swing._Phase * PI);
                        }
                    }

                    auto Rotation = FQuat::Slerp(State._Swing._StartRotation, LandingRotation, RotationAlpha);
                    if (NOT FMath::IsNearlyZero(_Settings._Swing._ToePitchDegrees))
                    {
                        const auto RightAxis = Rotation.GetAxisY();
                        Rotation = FQuat{RightAxis,
                            FMath::DegreesToRadians(_Settings._Swing._ToePitchDegrees) * ArcAlpha} * Rotation;
                    }

                    Out._Position = Position;
                    Out._Normal = State._Swing._TargetNormal;
                    Out._Rotation = Rotation;
                    Out._SwingAlpha = State._Swing._Phase;
                    Out._Planted = false;
                }
            }
            else
            {
                const auto Threshold = _Settings._Step._Threshold * FMath::Max(In._StepThresholdScale, KINDA_SMALL_NUMBER);
                const auto Error = FVector::Dist(State._Plant._Position, In._IdealTarget);

                const auto Wants = In._TargetValid && Error > Threshold;
                const auto Emergency = In._TargetValid && DoGet_IsEmergency(State, In);
                const auto Budget = _Settings._Cadence._MaxSimultaneousSwings <= 0 || NumSwinging < _Settings._Cadence._MaxSimultaneousSwings;

                const auto SettleWants = SettleActive && In._TargetValid
                    && Error > Threshold * FMath::Clamp(_Settings._Settle._ThresholdFraction, 0.05f, 1.0f);

                const auto CatchStep = State._PendingStep._Time > FCk_Time{};
                const auto YieldsToPriority = PriorityPhaseOffset.IsSet() && NOT CatchStep
                    && NOT FMath::IsNearlyEqual(PriorityPhaseOffset.GetValue(), LegPhaseOffset, 1.0e-3f);

                const auto Triggered = CatchStep || Emergency || (Wants && IsWindowOpen(LegPhaseOffset) && Budget) || (SettleWants && Budget);
                const auto OnSchedule = NOT IsInhibited(LegPhaseOffset) && NOT YieldsToPriority && Triggered;
                // A body climbing away from its planted feet stretches every group's floor feet at once, and the schedule
                // steps them one group after another. A foot past its chain steps now, inhibited or yielding, as long as no
                // other group swings beyond the schedule and the budget allows. So does a foot its hip cannot see: the body
                // carries it deeper behind the solid for every frame it waits, and at a walker's travel speed the other
                // group's reach Emergencies would keep its ratio of 1 waiting for longer than a step.
                const auto StepsBeyondSchedule = DoGet_IsHardOverstretched(State, In) || (In._PlantOccluded && In._TargetValid);
                const auto BeyondSchedule = NOT OnSchedule && NOT CatchStep && Budget && StepsBeyondSchedule
                    && (BeyondScheduleOffsets.IsEmpty() || ContainsGroup(BeyondScheduleOffsets, LegPhaseOffset));

                if (Advance && (OnSchedule || BeyondSchedule))
                {
                    const auto TargetTrusted = NOT CatchStep && In._TargetValid && In._TargetTrusted;
                    const auto SwingTarget = CatchStep
                        ? DoClampToReach(In, State._PendingStep._Target)
                        : DoClampTarget(In, In._TargetValid ? In._IdealTarget : State._Plant._Position, TargetTrusted);
                    DoBeginSwing(State, State._Plant._Position, State._Plant._Rotation, SwingTarget, TargetTrusted,
                        In._TargetValid ? In._GroundNormal : State._Plant._Normal);
                    State._Swing._TargetFrozen = false;
                    State._Swing._TargetOnAFace = NOT CatchStep && In._TargetValid && Get_IsFaceNormal(In._GroundNormal, FVector::UpVector);

                    State._Swing._Overshoot = NOT CatchStep && (Wants || Emergency) && NOT In._TargetIsFoothold
                        && NOT State._Swing._TargetOnAFace;

                    State._Swing._CatchStep = CatchStep;
                    State._Swing._BeyondSchedule = NOT OnSchedule;
                    State._PendingStep._Time = FCk_Time{};
                    ++NumSwinging;
                    (OnSchedule ? ScheduledOffsets : BeyondScheduleOffsets).Add(LegPhaseOffset);

                    Out._Position = State._Plant._Position;
                    Out._Normal = State._Plant._Normal;
                    Out._Rotation = State._Plant._Rotation;
                    Out._SwingAlpha = 0.0f;
                    Out._Planted = false;
                }
                else
                {
                    Out._Position = State._Plant._Position;
                    Out._Normal = State._Plant._Normal;
                    Out._Rotation = State._Plant._Rotation;
                    Out._SwingAlpha = 0.0f;
                    Out._Planted = true;
                }
            }
        }
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoReconcileEnabled(
            FProceduralGaitLegState& InOutState,
            const FProceduralGaitLegInput& InInput) const
        -> bool
    {
        if (InOutState._Enabled == InInput._Enabled)
        { return false; }

        if (NOT InInput._Enabled)
        {
            InOutState._Swing._Active = false;
            InOutState._Swing._TargetFrozen = false;
            InOutState._Swing._Overshoot = false;
            InOutState._Swing._CatchStep = false;
            InOutState._Swing._BeyondSchedule = false;
            InOutState._PendingStep._Time = FCk_Time{};
            InOutState._Plant._Position = InOutState._Emitted._Position;
            InOutState._Plant._Rotation = InOutState._Emitted._Rotation;
            InOutState._Plant._Trusted = false;
            InOutState._Enabled = false;
            return true;
        }

        InOutState._Enabled = true;
        // Take-off skips disabled legs, so a leg re-enabled mid-air would otherwise tuck from a stale air pose.
        InOutState._Emitted._AirPosition = InOutState._Emitted._Position;

        const auto Target = DoClampTarget(InInput, InInput._IdealTarget, InInput._TargetTrusted);
        const auto TargetIsAway = InInput._TargetValid
            && FVector::DistSquared(InOutState._Emitted._Position, Target) > KINDA_SMALL_NUMBER;

        if (TargetIsAway)
        {
            DoBeginSwing(InOutState, InOutState._Emitted._Position, InOutState._Emitted._Rotation, Target, InInput._TargetTrusted,
                InInput._GroundNormal);
            return true;
        }

        InOutState._Plant._Position = InOutState._Emitted._Position;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoRedistributeOffsets()
        -> void
    {
        const auto EnabledCount = NumEnabledLegs();
        _PatternBlend._RedistributedOffsets.SetNum(_LegStates.Num());

        auto Rank = 0;
        for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
        {
            if (NOT _LegStates[LegIndex]._Enabled)
            { continue; }

            _PatternBlend._RedistributedOffsets[LegIndex] = static_cast<float>(Rank) / static_cast<float>(EnabledCount);
            ++Rank;
        }
        _PatternBlend._HasRedistributedOffsets = true;

        _PatternBlend._FromOffsets = _PatternBlend._EffectiveOffsets;
        _PatternBlend._FromCycleScale = _PatternBlend._EffectiveCycleScale;
        _PatternBlend._Alpha = 0.0f;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoClearRedistribution()
        -> void
    {
        _PatternBlend._HasRedistributedOffsets = false;
        _PatternBlend._RedistributedOffsets.Reset();

        _PatternBlend._FromOffsets = _PatternBlend._EffectiveOffsets;
        _PatternBlend._FromCycleScale = _PatternBlend._EffectiveCycleScale;
        _PatternBlend._Alpha = 0.0f;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoBeginSwing(
            FProceduralGaitLegState& InOutState,
            const FVector& InStartPosition,
            const FQuat& InStartRotation,
            const FVector& InTarget,
            bool InTargetTrusted,
            const FVector& InTargetNormal)
        -> void
    {
        InOutState._Swing._Active = true;
        InOutState._Swing._Phase = 0.0f;
        InOutState._Swing._DurationScale = 1.0f;
        InOutState._Swing._StartPosition = InStartPosition;
        InOutState._Swing._StartRotation = InStartRotation;
        InOutState._Swing._Target = InTarget;
        // The ECS probes under the landing point before the next solve; a point left from the last swing would report the
        // ground the foot just lifted from.
        InOutState._Swing._LandingPoint = InTarget;
        InOutState._Swing._LandingLiftStartAlpha = -1.0f;
        InOutState._Swing._BeyondSchedule = false;
        InOutState._Swing._ValidatedTarget = InTarget;
        InOutState._Swing._LandsOnTarget = false;
        InOutState._Swing._PullBackStartAlpha = -1.0f;
        InOutState._Swing._TargetTrusted = InTargetTrusted;
        InOutState._Swing._TargetNormal = InTargetNormal;
        InOutState._Swing._TargetOnAFace = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoGet_SwingEase(
            float InPhase) const
        -> float
    {
        return _Settings._Swing._Profile.IsEaseValid()
            ? _Settings._Swing._Profile.SampleEase(InPhase)
            : FMath::SmoothStep(0.0f, 1.0f, InPhase);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoGet_FrozenTarget(
            const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput,
            const FVector& InBodyPlanarVelocity,
            FCk_Time InRemainingTime) const
        -> FVector
    {
        // A foothold was validated where it lies; pushed along the travel, the foot would land on ground nobody checked.
        if (InInput._TargetIsFoothold || InState._Swing._LandsOnTarget || InState._Swing._TargetOnAFace)
        { return DoClampTarget(InInput, InInput._IdealTarget, InInput._TargetTrusted); }

        return DoGet_DisplacedTarget(InInput, InInput._IdealTarget, InBodyPlanarVelocity * InRemainingTime.Get_Seconds(),
            InInput._TargetTrusted);
    }

    // --------------------------------------------------------------------------------------------------------------------

    // The swing target carried on along its stroke by the overshoot, then held within reach of where the hip is now: the hip
    // moves during the swing. InTarget is the target without the freeze push, InSwingTarget the one with it; the push and
    // the overshoot are displacements of InTarget, clamped together (see DoGet_DisplacedTarget).
    auto
        FProceduralGaitSolver::
        DoGet_LandingPoint(
            const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput,
            const FVector& InTarget,
            const FVector& InSwingTarget) const
        -> FVector
    {
        auto Displacement = InSwingTarget - InTarget;
        if (InState._Swing._Overshoot && _Settings._Step._StrokeOvershootFraction > 0.0f)
        {
            const auto Stroke = InSwingTarget - InState._Swing._StartPosition;
            const auto StrokeLength = Stroke.Size();
            if (StrokeLength > KINDA_SMALL_NUMBER)
            {
                const auto Overshoot = FMath::Min(
                    StrokeLength * _Settings._Step._StrokeOvershootFraction,
                    FMath::Max(_Settings._Step._MaxStrokeOvershoot, 0.0f));
                Displacement += (Stroke / StrokeLength) * Overshoot;
            }
        }
        return DoGet_DisplacedTarget(InInput, InTarget, Displacement, InState._Swing._TargetTrusted);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        Get_IsFaceNormal(
            const FVector& InNormal,
            const FVector& InUp)
        -> bool
    {
        return FVector::DotProduct(InNormal.GetSafeNormal(), InUp.GetSafeNormal())
            < FMath::Cos(FMath::DegreesToRadians(static_cast<double>(ProceduralGaitFaceAngleDegrees)));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        ClampToReach(
            const FVector& InHip,
            const FVector& InTarget,
            float InMaxDistance)
        -> FVector
    {
        const auto Offset = InTarget - InHip;
        const auto MaxDistance = static_cast<double>(InMaxDistance);
        if (MaxDistance <= 0.0 || Offset.SizeSquared() <= FMath::Square(MaxDistance))
        { return InTarget; }

        const auto Height = Offset.Z;
        if (FMath::Abs(Height) >= MaxDistance)
        { return InHip + Offset.GetSafeNormal() * MaxDistance; }

        const auto PlanarDistance = FMath::Sqrt(FMath::Square(MaxDistance) - FMath::Square(Height));
        return InHip + FVector{Offset.X, Offset.Y, 0.0}.GetSafeNormal() * PlanarDistance + FVector{0.0, 0.0, Height};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        ComputeLeadQuery(
            const FVector& InNeutral,
            const FVector& InLinearLead,
            const FVector& InPivot,
            const FVector& InUp,
            float InYawRate,
            FCk_Time InLeadTime,
            float InMaxLead)
        -> FVector
    {
        const auto Turn = FQuat{InUp.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector), InYawRate * InLeadTime.Get_Seconds()};
        const auto Led = InPivot + Turn.RotateVector(InNeutral + InLinearLead - InPivot);
        return InNeutral + (Led - InNeutral).GetClampedToMaxSize(FMath::Max(InMaxLead, 0.0f));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoClampToReach(
            const FProceduralGaitLegInput& InInput,
            const FVector& InTarget) const
        -> FVector
    {
        if (InInput._Reach <= 0.0f)
        { return InTarget; }

        return ClampToReach(InInput._Hip, InTarget, _Settings._Reach._TargetFraction * InInput._Reach);
    }

    // --------------------------------------------------------------------------------------------------------------------

    // A target on ground the caller validated stays on that ground while it lies within the force-step reach: pulled toward
    // the hip, a target near the reach limit would leave its tread or top for the air beside it. The reach Emergency steps
    // the leg again once the body has moved on. Anything else is held within the target reach.
    auto
        FProceduralGaitSolver::
        DoClampTarget(
            const FProceduralGaitLegInput& InInput,
            const FVector& InTarget,
            bool InTrusted) const
        -> FVector
    {
        const auto ForceStepLimit = static_cast<double>(_Settings._Reach._ForceStepFraction * InInput._Reach);
        if (InTrusted && InInput._Reach > 0.0f && FVector::DistSquared(InTarget, InInput._Hip) <= FMath::Square(ForceStepLimit))
        { return InTarget; }

        return DoClampToReach(InInput, InTarget);
    }

    // --------------------------------------------------------------------------------------------------------------------

    // A target displaced by the stroke overshoot and the freeze push is held within the target reach as a whole, pulled toward
    // the hip: the landing-ground probe under the landing point then verifies it, and with nothing there the swing drops the
    // displacement or pulls back to the target. A target without a displacement is held as DoClampTarget holds it, so ground
    // the caller validated stays where it is.
    auto
        FProceduralGaitSolver::
        DoGet_DisplacedTarget(
            const FProceduralGaitLegInput& InInput,
            const FVector& InTarget,
            const FVector& InDisplacement,
            bool InTrusted) const
        -> FVector
    {
        if (InDisplacement.SizeSquared() <= UE_DOUBLE_SMALL_NUMBER)
        { return DoClampTarget(InInput, InTarget, InTrusted); }

        return DoClampToReach(InInput, InTarget + InDisplacement);
    }

    // --------------------------------------------------------------------------------------------------------------------

    // A trusted target beyond the force-step reach is ground the leg cannot stand on from here: it counts as no target. A
    // target on a face is landed on where it lies, like a searched foothold.
    auto
        FProceduralGaitSolver::
        DoGet_EffectiveInput(
            const FProceduralGaitLegInput& InInput) const
        -> FProceduralGaitLegInput
    {
        auto Effective = InInput;
        const auto ForceStepLimit = static_cast<double>(_Settings._Reach._ForceStepFraction * InInput._Reach);
        if (InInput._TargetValid && InInput._TargetTrusted && InInput._Reach > 0.0f
            && FVector::DistSquared(InInput._IdealTarget, InInput._Hip) > FMath::Square(ForceStepLimit))
        { Effective._TargetValid = false; }
        return Effective;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoGet_IsEmergency(
            const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput) const
        -> bool
    {
        if (InInput._PlantOccluded && InInput._TargetValid)
        { return true; }

        const auto Threshold = _Settings._Step._Threshold * FMath::Max(InInput._StepThresholdScale, KINDA_SMALL_NUMBER);
        const auto Error = FVector::Dist(InState._Plant._Position, InInput._IdealTarget);
        if (Error > Threshold * FMath::Max(_Settings._Step._EmergencyFactor, 1.0f))
        { return true; }

        return InInput._Reach > 0.0f
            && FVector::Dist(InState._Plant._Position, InInput._Hip) > _Settings._Reach._ForceStepFraction * InInput._Reach;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoGet_IsHardOverstretched(
            const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput) const
        -> bool
    {
        return InInput._TargetValid && InInput._Reach > 0.0f
            && FVector::Dist(InState._Plant._Position, InInput._Hip) > _Settings._Reach._HardOverstretchFraction * InInput._Reach;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoGet_EmergencyRatio(
            const FProceduralGaitLegState& InState,
            const FProceduralGaitLegInput& InInput) const
        -> double
    {
        const auto Threshold = _Settings._Step._Threshold * FMath::Max(InInput._StepThresholdScale, KINDA_SMALL_NUMBER);
        const auto ErrorRatio = FVector::Dist(InState._Plant._Position, InInput._IdealTarget)
            / (Threshold * FMath::Max(_Settings._Step._EmergencyFactor, 1.0f));
        // An occluded plant counts as an Emergency at its trigger, so an over-reach or a large error still outranks it in the
        // priority choice; it steps beyond the schedule either way.
        constexpr auto OccludedPlantRatio = 1.0;
        const auto BaseRatio = InInput._PlantOccluded ? FMath::Max(ErrorRatio, OccludedPlantRatio) : ErrorRatio;
        if (InInput._Reach <= 0.0f)
        { return BaseRatio; }

        return FMath::Max(BaseRatio,
            FVector::Dist(InState._Plant._Position, InInput._Hip) / (_Settings._Reach._ForceStepFraction * InInput._Reach));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralGaitSolver::
        DoWriteDisabledOutput(
            const FProceduralGaitLegState& InState,
            FProceduralGaitLegOutput& OutOutput)
        -> void
    {
        OutOutput._Position = InState._Emitted._Position;
        OutOutput._Normal = InState._Plant._Normal;
        OutOutput._Rotation = InState._Emitted._Rotation;
        OutOutput._SwingAlpha = 0.0f;
        OutOutput._Planted = true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
