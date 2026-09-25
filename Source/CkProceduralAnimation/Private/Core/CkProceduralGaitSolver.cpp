#include "CkProceduralAnimation/Core/CkProceduralGaitSolver.h"

namespace ck::procedural_gait_solver
{
    auto Damp(const FVector& InCurrent, const FVector& InTarget, float InRate, FCk_Time InDeltaTime) -> FVector
    {
        const auto Alpha = 1.0 - FMath::Exp(-InRate * InDeltaTime.Get_Seconds());
        return FMath::Lerp(InCurrent, InTarget, Alpha);
    }
    auto Damp(const FQuat& InCurrent, const FQuat& InTarget, float InRate, FCk_Time InDeltaTime) -> FQuat
    {
        const auto Alpha = 1.0 - FMath::Exp(-InRate * InDeltaTime.Get_Seconds());
        return FQuat::Slerp(InCurrent, InTarget, Alpha).GetNormalized();
    }
}

namespace ck
{
    auto
        FProceduralGaitVelocityTracker::
        Reset()
        -> void
    {
        _Samples.Reset();
        _NextSample = 0;
        _HasPrevious = false;
    }

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
        for (const FVector& Sample : _Samples)
        {
            Sum += Sample;
        }
        return Sum / static_cast<float>(_Samples.Num());
    }

    auto
        FProceduralGaitSolver::
        Reset(
            TArrayView<const FVector> InInitialFootPositions)
        -> bool
    {
        for (const auto& Position : InInitialFootPositions)
        {
            if (Position.ContainsNaN())
            { return false; }
        }

        _LegStates.Reset(InInitialFootPositions.Num());
        for (const FVector& Position : InInitialFootPositions)
        {
            auto& State = _LegStates.AddDefaulted_GetRef();
            State._PlantedPosition = Position;
            State._AirPosition = Position;
            State._CurrentPosition = Position;
            State._CurrentRotation = State._PlantedRotation;
        }
        _GaitClock = 0.f;
        _LastCadenceScale = 1.0f;
        _RestTime = FCk_Time{};
        _WasAirborne = false;
        _CurrentPatternIndex = INDEX_NONE;
        _PatternBlendAlpha = 1.f;
        _EffectiveCycleScale = 1.f;
        _BlendFromCycleScale = 1.f;
        _EffectivePhaseOffsets.Reset();
        _BlendFromOffsets.Reset();
        return true;
    }

    auto
        FProceduralGaitSolver::
        MakeFootRotation(
            const FVector& InFacingDirection,
            const FVector& InGroundNormal)
        -> FQuat
    {
        const auto Z = InGroundNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
        auto X = InFacingDirection.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);

        if (FMath::Abs(FVector::DotProduct(X, Z)) > 1.f - KINDA_SMALL_NUMBER)
        {
            const auto Reference = FMath::Abs(Z.Y) < 0.9f ? FVector::RightVector : FVector::ForwardVector;
            X = FVector::CrossProduct(Z, Reference);
        }
        return FRotationMatrix::MakeFromZX(Z, X).ToQuat();
    }

    auto
        FProceduralGaitSolver::
        ComputeTraceAxis(
            const FVector& InBodyUp,
            const FVector& InRadialDirection,
            float InOutwardLean)
        -> FVector
    {
        const auto Up = InBodyUp.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
        const auto Lean = FMath::Clamp(InOutwardLean, 0.f, 1.f);
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

    auto
        FProceduralGaitSolver::
        SetPlantedPose(
            int32 InLegIndex,
            const FVector& InPosition,
            const FVector& InNormal)
        -> void
    {
        if (InPosition.ContainsNaN() || InNormal.ContainsNaN() || NOT InNormal.IsNormalized())
        { return; }
        if (_LegStates.IsValidIndex(InLegIndex) && NOT _LegStates[InLegIndex]._Swinging)
        {
            auto& State = _LegStates[InLegIndex];

            const auto NormalDelta = FQuat::FindBetweenNormals(
                State._PlantedNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector),
                InNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector));
            State._PlantedRotation = NormalDelta * State._PlantedRotation;
            State._PlantedPosition = InPosition;
            State._PlantedNormal = InNormal;
            State._CurrentPosition = State._PlantedPosition;
            State._CurrentRotation = State._PlantedRotation;
        }
    }

    auto
        FProceduralGaitSolver::
        RequestStep(
            int32 InLegIndex,
            const FVector& InTarget)
        -> bool
    {
        if (NOT _LegStates.IsValidIndex(InLegIndex) || InTarget.ContainsNaN() || NOT ValidateSettings(_Settings))
        {
            return false;
        }

        auto& State = _LegStates[InLegIndex];
        State._PendingStepTarget = InTarget;

        State._PendingStepTime = _Settings._CatchStepLifetime > FCk_Time{}
            ? _Settings._CatchStepLifetime
            : FMath::Max(_Settings._CycleDuration, FCk_Time{KINDA_SMALL_NUMBER});
        return true;
    }

    auto
        FProceduralGaitSolver::
        TransformState(
            const FQuat& InDelta)
        -> void
    {
        if (InDelta.ContainsNaN() || NOT InDelta.IsNormalized())
        { return; }
        for (FProceduralGaitLegState& State : _LegStates)
        {
            State._PendingStepTarget = InDelta.RotateVector(State._PendingStepTarget);
            State._PlantedPosition = InDelta.RotateVector(State._PlantedPosition);
            State._PlantedNormal = InDelta.RotateVector(State._PlantedNormal);
            State._SwingStartPosition = InDelta.RotateVector(State._SwingStartPosition);
            State._SwingTarget = InDelta.RotateVector(State._SwingTarget);
            State._AirPosition = InDelta.RotateVector(State._AirPosition);
            State._CurrentPosition = InDelta.RotateVector(State._CurrentPosition);
            State._CurrentRotation = InDelta * State._CurrentRotation;
            State._PlantedRotation = InDelta * State._PlantedRotation;
            State._SwingStartRotation = InDelta * State._SwingStartRotation;
        }
    }

    auto
        FProceduralGaitSolver::
        IsWindowOpen(
            float InPhaseOffset) const
        -> bool
    {
        const auto Local = FMath::Frac(_GaitClock - InPhaseOffset + 1.f);
        return Local < _Settings._SwingWindow;
    }

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
        _EffectivePhaseOffsets.SetNum(NumInputs);

        if (_Settings._Patterns.Num() == 0)
        {
            for (auto LegIndex = 0; LegIndex < NumInputs; ++LegIndex)
            {
                _EffectivePhaseOffsets[LegIndex] = InInputs[LegIndex]._PhaseOffset;
            }
            _CurrentPatternIndex = INDEX_NONE;
            _PatternBlendAlpha = 1.f;
            _EffectiveCycleScale = 1.f;
            return;
        }

        auto Desired = int32{INDEX_NONE};
        auto DesiredMinSpeed = -FLT_MAX;
        auto Slowest = 0;
        auto SlowestMinSpeed = FLT_MAX;
        for (auto PatternIndex = 0; PatternIndex < _Settings._Patterns.Num(); ++PatternIndex)
        {
            const auto MinSpeed = _Settings._Patterns[PatternIndex]._MinSpeed;
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

        if (NOT _Settings._Patterns.IsValidIndex(_CurrentPatternIndex))
        {
            _CurrentPatternIndex = Desired;
            _PatternBlendAlpha = 1.f;
            _BlendFromOffsets.Reset();
            _BlendFromCycleScale = _Settings._Patterns[Desired]._CycleDurationScale;
        }
        else if (Desired != _CurrentPatternIndex && InAdvance)
        {
            const auto Downward =
                _Settings._Patterns[Desired]._MinSpeed < _Settings._Patterns[_CurrentPatternIndex]._MinSpeed;
            const auto Allowed = NOT Downward
                || InBodyPlanarSpeed < _Settings._Patterns[_CurrentPatternIndex]._MinSpeed
                    * FMath::Clamp(_Settings._PatternSwitchHysteresis, 0.f, 1.f);
            if (Allowed)
            {
                _BlendFromOffsets = _EffectivePhaseOffsets;
                _BlendFromCycleScale = _EffectiveCycleScale;
                _PatternBlendAlpha = 0.f;
                _CurrentPatternIndex = Desired;
            }
        }

        if (InAdvance)
        {
            _PatternBlendAlpha = FMath::Min(1.f,
                _PatternBlendAlpha + static_cast<float>(InDeltaTime / FMath::Max(_Settings._PatternBlendTime, FCk_Time{KINDA_SMALL_NUMBER})));
        }

        const auto& Pattern = _Settings._Patterns[_CurrentPatternIndex];
        const auto Alpha = FMath::SmoothStep(0.f, 1.f, _PatternBlendAlpha);
        for (auto LegIndex = 0; LegIndex < NumInputs; ++LegIndex)
        {
            const auto Target = Pattern._PhaseOffsets.IsValidIndex(LegIndex)
                ? Pattern._PhaseOffsets[LegIndex]
                : InInputs[LegIndex]._PhaseOffset;
            const auto From = _BlendFromOffsets.IsValidIndex(LegIndex) ? _BlendFromOffsets[LegIndex] : Target;

            const auto Delta = FMath::Frac(Target - From + 1.5f) - 0.5f;
            _EffectivePhaseOffsets[LegIndex] = FMath::Frac(From + Delta * Alpha + 1.f);
        }
        _EffectiveCycleScale = FMath::Lerp(_BlendFromCycleScale, Pattern._CycleDurationScale, Alpha);
    }

    auto
        FProceduralGaitSolver::
        ValidateSettings(
            const FProceduralGaitSettings& InSettings)
        -> bool
    {
        const float ScalarValues[] =
        {
            InSettings._StepThreshold,
            InSettings._EmergencyStepFactor,
            InSettings._StepHeight,
            InSettings._SwingWindow,
            InSettings._MoveSpeedThreshold,
            InSettings._CadenceSpeedRef,
            InSettings._MaxCadenceScale,
            InSettings._RetargetSmoothing,
            InSettings._RetargetFreezePhase,
            InSettings._SwingApexPhase,
            InSettings._SwingApexSharpness,
            InSettings._SprintApexHeightScale,
            InSettings._ObstacleClearance,
            InSettings._StrokeOvershootFraction,
            InSettings._MaxStrokeOvershoot,
            InSettings._ScheduleAdvanceFraction,
            InSettings._ScheduleAdvanceRate,
            InSettings._SettleThresholdFraction,
            InSettings._AirborneTuckLift,
            InSettings._AirborneFollowSpeed,
            InSettings._LandingStepDurationScale,
            InSettings._PatternSwitchHysteresis,
            InSettings._SwingToePitchDegrees,
        };
        for (const auto Value : ScalarValues)
        {
            if (NOT FMath::IsFinite(Value))
            { return false; }
        }
        const FCk_Time Durations[] =
        {
            InSettings._StepDuration,
            InSettings._CycleDuration,
            InSettings._CatchStepLifetime,
            InSettings._SettleDelay,
            InSettings._PatternBlendTime,
        };
        for (const auto Duration : Durations)
        {
            if (NOT FMath::IsFinite(Duration.Get_Seconds()) || Duration < FCk_Time{})
            { return false; }
        }
        const auto UnitInterval = [](float InValue) { return InValue >= 0.0f && InValue <= 1.0f; };
        const auto PositiveUnitInterval = [](float InValue) { return InValue > 0.0f && InValue <= 1.0f; };
        if (InSettings._StepThreshold <= 0.0f || InSettings._StepDuration <= FCk_Time{}
            || InSettings._CycleDuration <= FCk_Time{} || InSettings._StepHeight < 0.0f
            || InSettings._EmergencyStepFactor < 1.0f || NOT PositiveUnitInterval(InSettings._SwingWindow)
            || InSettings._MoveSpeedThreshold < 0.0f || InSettings._CadenceSpeedRef < 0.0f
            || InSettings._MaxCadenceScale < 1.0f || InSettings._RetargetSmoothing < 0.0f
            || NOT UnitInterval(InSettings._RetargetFreezePhase) || InSettings._MaxSimultaneousSwings < 0
            || NOT UnitInterval(InSettings._SwingApexPhase) || InSettings._SwingApexSharpness <= 0.0f
            || InSettings._SprintApexHeightScale < 0.0f || InSettings._ObstacleClearance < 0.0f
            || InSettings._StrokeOvershootFraction < 0.0f || InSettings._MaxStrokeOvershoot < 0.0f
            || NOT UnitInterval(InSettings._ScheduleAdvanceFraction) || InSettings._ScheduleAdvanceRate < 0.0f
            || NOT PositiveUnitInterval(InSettings._SettleThresholdFraction) || InSettings._AirborneTuckLift < 0.0f
            || InSettings._AirborneFollowSpeed < 0.0f || NOT PositiveUnitInterval(InSettings._LandingStepDurationScale)
            || NOT PositiveUnitInterval(InSettings._PatternSwitchHysteresis))
        { return false; }
        for (const auto& Pattern : InSettings._Patterns)
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
                || NOT FMath::IsFinite(Input._ClearanceGroundZ)
                || (Input._TargetValid && NOT Input._GroundNormal.IsNormalized()))
            { return false; }
        }

        if (InDeltaTime <= FCk_Time{})
        {
            auto ReadOnlySolver = *this;
            return ReadOnlySolver.DoStep(InDeltaTime, InBodyPlanarSpeed, InBodyPlanarVelocity,
                InInputs, OutOutputs, InAirborne);
        }
        const auto Advanced = DoStep(InDeltaTime, InBodyPlanarSpeed, InBodyPlanarVelocity, InInputs, OutOutputs, InAirborne);
        if (Advanced)
        {
            for (auto LegIndex = 0; LegIndex < OutOutputs.Num(); ++LegIndex)
            {
                _LegStates[LegIndex]._CurrentPosition = OutOutputs[LegIndex]._Position;
                _LegStates[LegIndex]._CurrentRotation = OutOutputs[LegIndex]._Rotation;
            }
        }
        return Advanced;
    }

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

        if (Advance)
        {
            for (FProceduralGaitLegState& State : _LegStates)
            {
                if (State._PendingStepTime > FCk_Time{})
                {
                    State._PendingStepTime = FMath::Max(State._PendingStepTime - InDeltaTime, FCk_Time{});
                }
            }
        }

        UpdatePatternSelection(InDeltaTime, InBodyPlanarSpeed, InInputs, Advance);

        auto CadenceScale = 1.f;
        if (_Settings._CadenceSpeedRef > KINDA_SMALL_NUMBER)
        {
            CadenceScale = FMath::Clamp(InBodyPlanarSpeed / _Settings._CadenceSpeedRef, 1.f, FMath::Max(_Settings._MaxCadenceScale, 1.f));
        }

        if (Advance)
        { _LastCadenceScale = CadenceScale; }

        const auto Moving = InBodyPlanarSpeed > _Settings._MoveSpeedThreshold;

        if (Advance)
        {
            _RestTime = (NOT Moving && NOT InAirborne) ? _RestTime + InDeltaTime : FCk_Time{};
        }
        const auto SettleActive = _Settings._SettleAtRest && _RestTime >= _Settings._SettleDelay;

        if (Advance && Moving && NOT InAirborne)
        {
            _GaitClock = FMath::Frac(_GaitClock + static_cast<float>(InDeltaTime * CadenceScale
                / FMath::Max(_Settings._CycleDuration * _EffectiveCycleScale, FCk_Time{KINDA_SMALL_NUMBER})));
        }

        if (Advance && InAirborne && NOT _WasAirborne)
        {
            for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
            {
                auto& State = _LegStates[LegIndex];

                State._AirPosition = State._CurrentPosition;
                State._PlantedRotation = State._CurrentRotation;
                State._Swinging = false;
                State._SwingPhase = 0.f;
                State._TargetFrozen = false;
                State._Overshoot = false;
                State._CatchStep = false;
                State._SwingDurationScale = 1.f;
            }
        }
        else if (Advance && NOT InAirborne && _WasAirborne)
        {
            for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
            {
                auto& State = _LegStates[LegIndex];
                const auto& In = InInputs[LegIndex];
                State._Swinging = true;
                State._SwingPhase = 0.f;
                State._SwingStartPosition = State._AirPosition;
                State._SwingStartRotation = State._PlantedRotation;
                State._SwingTarget = In._TargetValid ? In._IdealTarget : State._AirPosition;
                State._TargetFrozen = false;

                State._Overshoot = false;

                State._CatchStep = false;
                State._SwingDurationScale = FMath::Clamp(_Settings._LandingStepDurationScale, 0.1f, 1.f);
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

                const auto TuckTarget = In._IdealTarget + FVector(0.f, 0.f, _Settings._AirborneTuckLift);
                const auto TuckRotation = MakeFootRotation(In._FacingDirection, FVector::UpVector);
                if (Advance)
                {
                    State._AirPosition = procedural_gait_solver::Damp(State._AirPosition, TuckTarget, FMath::Max(_Settings._AirborneFollowSpeed, KINDA_SMALL_NUMBER), InDeltaTime);
                    State._PlantedRotation = procedural_gait_solver::Damp(State._PlantedRotation, TuckRotation,
                        FMath::Max(_Settings._AirborneFollowSpeed, KINDA_SMALL_NUMBER), InDeltaTime);
                }

                Out._Position = State._AirPosition;
                Out._Normal = FVector::UpVector;
                Out._Rotation = State._PlantedRotation;
                Out._SwingAlpha = 1.f;
                Out._Planted = false;
            }
            return true;
        }

        auto NumSwinging = 0;
        TArray<float, TInlineAllocator<8>> SwingingOffsets;
        for (auto LegIndex = 0; LegIndex < _LegStates.Num(); ++LegIndex)
        {
            if (_LegStates[LegIndex]._Swinging)
            {
                ++NumSwinging;
                SwingingOffsets.Add(_EffectivePhaseOffsets[LegIndex]);
            }
        }

        auto IsInhibited = [&SwingingOffsets](float PhaseOffset)
        {
            for (const float Offset : SwingingOffsets)
            {
                if (NOT FMath::IsNearlyEqual(Offset, PhaseOffset, 1.e-3f))
                {
                    return true;
                }
            }
            return false;
        };

        if (Advance && _Settings._ScheduleAdvanceFraction > 0.f && _Settings._ScheduleAdvanceRate > 0.f)
        {
            const auto AdvanceTrigger = FMath::Max(_Settings._EmergencyStepFactor, 1.f)
                * FMath::Clamp(_Settings._ScheduleAdvanceFraction, 0.f, 1.f);

            auto WorstRatio = AdvanceTrigger;
            auto WorstGap = 0.f;
            for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
            {
                const auto& In = InInputs[LegIndex];
                const auto& State = _LegStates[LegIndex];
                const auto Offset = _EffectivePhaseOffsets[LegIndex];

                if (State._Swinging || NOT In._TargetValid || IsWindowOpen(Offset))
                {
                    continue;
                }

                const auto Threshold = _Settings._StepThreshold * FMath::Max(In._StepThresholdScale, KINDA_SMALL_NUMBER);
                const auto Ratio = FVector::Dist(State._PlantedPosition, In._IdealTarget) / Threshold;
                if (Ratio > WorstRatio)
                {
                    WorstRatio = Ratio;

                    WorstGap = FMath::Frac(Offset - _GaitClock + 1.f);
                }
            }

            if (WorstGap > 0.f)
            {
                _GaitClock = FMath::Frac(_GaitClock
                    + FMath::Min(WorstGap, static_cast<float>(InDeltaTime.Get_Seconds()) * _Settings._ScheduleAdvanceRate) + 1.f);
            }
        }

        for (auto LegIndex = 0; LegIndex < InInputs.Num(); ++LegIndex)
        {
            const auto& In = InInputs[LegIndex];
            auto& State = _LegStates[LegIndex];
            auto& Out = OutOutputs[LegIndex];
            const auto LegPhaseOffset = _EffectivePhaseOffsets[LegIndex];

            if (State._Swinging)
            {
                const auto SwingDuration = FMath::Max(_Settings._StepDuration * State._SwingDurationScale / CadenceScale, FCk_Time{KINDA_SMALL_NUMBER});
                if (Advance)
                {
                    State._SwingPhase = FMath::Min(State._SwingPhase + static_cast<float>(InDeltaTime / SwingDuration), 1.f);

                    if (NOT State._CatchStep && NOT State._TargetFrozen
                        && State._SwingPhase >= _Settings._RetargetFreezePhase && In._TargetValid)
                    {
                        const auto RemainingTime = SwingDuration * (1.f - State._SwingPhase);
                        State._SwingTarget = In._IdealTarget + InBodyPlanarVelocity * RemainingTime.Get_Seconds();
                        State._TargetFrozen = true;
                    }
                    else if (NOT State._CatchStep && NOT State._TargetFrozen)
                    {
                        const auto DesiredTarget = In._TargetValid ? In._IdealTarget : State._PlantedPosition;
                        State._SwingTarget = procedural_gait_solver::Damp(State._SwingTarget, DesiredTarget, FMath::Max(_Settings._RetargetSmoothing, KINDA_SMALL_NUMBER), InDeltaTime);
                    }
                }

                auto Target = State._SwingTarget;
                if (State._Overshoot && _Settings._StrokeOvershootFraction > 0.f)
                {
                    const auto Stroke = State._SwingTarget - State._SwingStartPosition;
                    const auto StrokeLength = Stroke.Size();
                    if (StrokeLength > KINDA_SMALL_NUMBER)
                    {
                        const auto Overshoot = FMath::Min(
                            StrokeLength * _Settings._StrokeOvershootFraction,
                            FMath::Max(_Settings._MaxStrokeOvershoot, 0.f));
                        Target += (Stroke / StrokeLength) * Overshoot;
                    }
                }

                const auto LandingRotation = MakeFootRotation(In._FacingDirection,
                    In._TargetValid ? In._GroundNormal : State._PlantedNormal);

                if (State._SwingPhase >= 1.f)
                {
                    State._Swinging = false;
                    State._SwingPhase = 0.f;
                    State._SwingDurationScale = 1.f;
                    State._Overshoot = false;

                    if (In._TargetValid && NOT State._CatchStep)
                    {
                        const auto BelowGround = FVector::DotProduct(In._IdealTarget - Target, In._GroundNormal);
                        if (BelowGround > 0.f)
                        {
                            Target += In._GroundNormal * BelowGround;
                        }
                    }
                    State._CatchStep = false;

                    State._PlantedPosition = Target;
                    State._PlantedNormal = In._TargetValid ? In._GroundNormal : State._PlantedNormal;
                    State._PlantedRotation = LandingRotation;

                    Out._Position = State._PlantedPosition;
                    Out._Normal = State._PlantedNormal;
                    Out._Rotation = State._PlantedRotation;
                    Out._SwingAlpha = 0.f;
                    Out._Planted = true;
                }
                else
                {
                    const auto Alpha = _Settings._SwingProfile.IsEaseValid()
                        ? _Settings._SwingProfile.SampleEase(State._SwingPhase)
                        : FMath::SmoothStep(0.f, 1.f, State._SwingPhase);
                    const auto RotationAlpha = FMath::Clamp(Alpha, 0.f, 1.f);
                    auto Position = FMath::Lerp(State._SwingStartPosition, Target, Alpha);

                    float ArcAlpha;
                    if (_Settings._SwingProfile.IsArcValid())
                    {
                        ArcAlpha = _Settings._SwingProfile.SampleArc(State._SwingPhase);
                    }
                    else
                    {
                        ArcAlpha = procedural_gait_swing::ParametricArc(
                            State._SwingPhase, _Settings._SwingApexPhase, _Settings._SwingApexSharpness);
                    }

                    auto HeightScale = 1.f;
                    if (_Settings._MaxCadenceScale > 1.f + KINDA_SMALL_NUMBER)
                    {
                        const auto SprintBlend = (CadenceScale - 1.f) / (_Settings._MaxCadenceScale - 1.f);
                        HeightScale = FMath::Lerp(1.f, _Settings._SprintApexHeightScale, FMath::Clamp(SprintBlend, 0.f, 1.f));
                    }
                    Position.Z += ArcAlpha * _Settings._StepHeight * HeightScale;

                    if (_Settings._ObstacleClearance > 0.f && In._ClearanceGroundZ > -FLT_MAX * 0.5f)
                    {
                        const auto Deficit = (In._ClearanceGroundZ + _Settings._ObstacleClearance) - Position.Z;
                        if (Deficit > 0.f)
                        {
                            Position.Z += Deficit * FMath::Sin(State._SwingPhase * PI);
                        }
                    }

                    auto Rotation = FQuat::Slerp(State._SwingStartRotation, LandingRotation, RotationAlpha);
                    if (NOT FMath::IsNearlyZero(_Settings._SwingToePitchDegrees))
                    {
                        const auto RightAxis = Rotation.GetAxisY();
                        Rotation = FQuat(RightAxis,
                            FMath::DegreesToRadians(_Settings._SwingToePitchDegrees) * ArcAlpha) * Rotation;
                    }

                    Out._Position = Position;
                    Out._Normal = In._TargetValid ? In._GroundNormal : State._PlantedNormal;
                    Out._Rotation = Rotation;
                    Out._SwingAlpha = State._SwingPhase;
                    Out._Planted = false;
                }
            }
            else
            {
                const auto Threshold = _Settings._StepThreshold * FMath::Max(In._StepThresholdScale, KINDA_SMALL_NUMBER);
                const auto Error = FVector::Dist(State._PlantedPosition, In._IdealTarget);

                const auto Wants = In._TargetValid && Error > Threshold;
                const auto Emergency = In._TargetValid && Error > Threshold * FMath::Max(_Settings._EmergencyStepFactor, 1.f);
                const auto Budget = _Settings._MaxSimultaneousSwings <= 0 || NumSwinging < _Settings._MaxSimultaneousSwings;

                const auto SettleWants = SettleActive && In._TargetValid
                    && Error > Threshold * FMath::Clamp(_Settings._SettleThresholdFraction, 0.05f, 1.f);

                const auto CatchStep = State._PendingStepTime > FCk_Time{};

                if (Advance && NOT IsInhibited(LegPhaseOffset) &&
                    (CatchStep || Emergency || (Wants && IsWindowOpen(LegPhaseOffset) && Budget) || (SettleWants && Budget)))
                {
                    State._Swinging = true;
                    State._SwingPhase = 0.f;
                    State._SwingDurationScale = 1.f;
                    State._SwingStartPosition = State._PlantedPosition;
                    State._SwingStartRotation = State._PlantedRotation;
                    State._SwingTarget = CatchStep
                        ? State._PendingStepTarget
                        : (In._TargetValid ? In._IdealTarget : State._PlantedPosition);
                    State._TargetFrozen = false;

                    State._Overshoot = NOT CatchStep && (Wants || Emergency);

                    State._CatchStep = CatchStep;
                    State._PendingStepTime = FCk_Time{};
                    ++NumSwinging;
                    SwingingOffsets.Add(LegPhaseOffset);

                    Out._Position = State._PlantedPosition;
                    Out._Normal = State._PlantedNormal;
                    Out._Rotation = State._PlantedRotation;
                    Out._SwingAlpha = 0.f;
                    Out._Planted = false;
                }
                else
                {
                    Out._Position = State._PlantedPosition;
                    Out._Normal = State._PlantedNormal;
                    Out._Rotation = State._PlantedRotation;
                    Out._SwingAlpha = 0.f;
                    Out._Planted = true;
                }
            }
        }
        return true;
    }
}
