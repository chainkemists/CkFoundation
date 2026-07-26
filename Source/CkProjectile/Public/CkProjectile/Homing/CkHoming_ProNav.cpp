#include "CkHoming_ProNav.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pronav_detail
{
    // Countered by the guidance so gravity/wind do not bend the intercept course
    static auto
    Get_PerpendicularComponent(
        const FVector& InAcceleration,
        const FVector& InPreservedDirection) -> FVector
    {
        return InAcceleration - InAcceleration.Dot(InPreservedDirection) * InPreservedDirection;
    }

    static auto
    Compute_TurnAndThrust_Acceleration(
        const FCk_Homing_GuidanceState& InState,
        const FCk_Homing_Kinematics& InKinematics,
        const FCk_Homing_GuidanceSettings& InSettings,
        const FVector& InExternalAcceleration,
        FCk_Time InDeltaT) -> FVector
    {
        const auto& LineOfSightDirection = InState.Get_LineOfSightDirection();
        const auto MaxAcceleration = InSettings.Get_MaxAcceleration();

        const auto IsReceding = InState.Get_ClosingSpeed() <= 0.0;

        if (IsReceding)
        {
            if (InSettings.Get_MaxSpeed() > 0.0f)
            {
                const auto VelocityTarget = InSettings.Get_MaxSpeed() * LineOfSightDirection;
                const auto NewVelocity = FMath::VInterpConstantTo(
                    InKinematics.Get_PursuerVelocity(), VelocityTarget, InDeltaT.Get_Seconds(), MaxAcceleration);

                return (NewVelocity - InKinematics.Get_PursuerVelocity()) / InDeltaT.Get_Seconds();
            }

            return MaxAcceleration * LineOfSightDirection;
        }

        const auto Steering = pronav::Compute_TrueProNav_Acceleration(InState, InSettings.Get_NavigationGain()) -
            Get_PerpendicularComponent(InExternalAcceleration, LineOfSightDirection);

        const auto MaxAccelerationSquared = FMath::Square(MaxAcceleration);
        const auto SteeringSizeSquared = Steering.SizeSquared();

        if (SteeringSizeSquared >= MaxAccelerationSquared)
        { return Steering.GetClampedToMaxSize(MaxAcceleration); }

        const auto RemainingAcceleration = FMath::Sqrt(MaxAccelerationSquared - SteeringSizeSquared);

        const auto DesiredTimeToImpact = InSettings.Get_DesiredTimeToImpact();

        if (InSettings.Get_ImpactTiming() == ECk_Homing_ImpactTiming::ArriveAtDesiredTime &&
            DesiredTimeToImpact > FCk_Time::ZeroSecond())
        {
            const auto RequiredClosingSpeed = InState.Get_LineOfSightDistance() / DesiredTimeToImpact.Get_Seconds();
            const auto ClosureCorrection = FMath::Clamp(
                (RequiredClosingSpeed - InState.Get_ClosingSpeed()) / InDeltaT.Get_Seconds(),
                -RemainingAcceleration, RemainingAcceleration);

            return Steering + LineOfSightDirection * ClosureCorrection;
        }

        return Steering + RemainingAcceleration * LineOfSightDirection;
    }

    static auto
    Compute_TurnOnly_Acceleration(
        const FCk_Homing_GuidanceState& InState,
        const FCk_Homing_Kinematics& InKinematics,
        const FCk_Homing_GuidanceSettings& InSettings,
        const FVector& InExternalAcceleration) -> FVector
    {
        const auto& PursuerVelocity = InKinematics.Get_PursuerVelocity();

        if (PursuerVelocity.IsNearlyZero())
        { return FVector::ZeroVector; }

        const auto VelocityDirection = PursuerVelocity.GetUnsafeNormal();

        const auto Steering = pronav::Compute_PureProNav_Acceleration(InState, PursuerVelocity, InSettings.Get_NavigationGain()) -
            Get_PerpendicularComponent(InExternalAcceleration, VelocityDirection);

        return Steering.GetClampedToMaxSize(InSettings.Get_MaxAcceleration());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pronav
{
    auto
        Compute_GuidanceState(
            const FCk_Homing_Kinematics& InKinematics)
        -> FCk_Homing_GuidanceState
    {
        const auto LineOfSight = InKinematics.Get_TargetLocation() - InKinematics.Get_PursuerLocation();
        const auto Distance = LineOfSight.Size();

        if (Distance < UE_DOUBLE_KINDA_SMALL_NUMBER)
        { return FCk_Homing_GuidanceState{}; }

        const auto RelativeVelocity = InKinematics.Get_TargetVelocity() - InKinematics.Get_PursuerVelocity();

        return FCk_Homing_GuidanceState{}
            .Set_LineOfSightDirection(LineOfSight / Distance)
            .Set_LineOfSightDistance(Distance)
            .Set_LineOfSightAngularVelocity(LineOfSight.Cross(RelativeVelocity) / FMath::Square(Distance))
            .Set_ClosingSpeed(-RelativeVelocity.Dot(LineOfSight) / Distance);
    }

    auto
        Compute_TrueProNav_Acceleration(
            const FCk_Homing_GuidanceState& InState,
            float InNavigationGain)
        -> FVector
    {
        return InNavigationGain * InState.Get_ClosingSpeed() *
            InState.Get_LineOfSightAngularVelocity().Cross(InState.Get_LineOfSightDirection());
    }

    auto
        Compute_PureProNav_Acceleration(
            const FCk_Homing_GuidanceState& InState,
            const FVector& InPursuerVelocity,
            float InNavigationGain)
        -> FVector
    {
        return InNavigationGain * InState.Get_LineOfSightAngularVelocity().Cross(InPursuerVelocity);
    }

    auto
        Compute_HomingAcceleration(
            const FCk_Homing_GuidanceState& InState,
            const FCk_Homing_Kinematics& InKinematics,
            const FCk_Homing_GuidanceSettings& InSettings,
            const FVector& InExternalAcceleration,
            FCk_Time InDeltaT)
        -> FVector
    {
        if (InSettings.Get_MaxAcceleration() <= 0.0f)
        { return FVector::ZeroVector; }

        if (InState.Get_LineOfSightDistance() < UE_DOUBLE_KINDA_SMALL_NUMBER)
        { return FVector::ZeroVector; }

        CK_ENSURE_IF_NOT(InDeltaT > FCk_Time::ZeroSecond(),
            TEXT("Compute_HomingAcceleration requires a positive DeltaT, got [{}]"), InDeltaT)
        { return FVector::ZeroVector; }

        switch (const auto SpeedControl = InSettings.Get_SpeedControl())
        {
            case ECk_Homing_SpeedControl::TurnOnly:
            {
                return pronav_detail::Compute_TurnOnly_Acceleration(
                    InState, InKinematics, InSettings, InExternalAcceleration);
            }
            case ECk_Homing_SpeedControl::TurnAndThrust:
            {
                return pronav_detail::Compute_TurnAndThrust_Acceleration(
                    InState, InKinematics, InSettings, InExternalAcceleration, InDeltaT);
            }
            default:
            {
                CK_INVALID_ENUM(SpeedControl);
                return FVector::ZeroVector;
            }
        }
    }

    auto
        Compute_FiringSolution(
            const FVector& InShooterLocation,
            const FVector& InShooterVelocity,
            const FVector& InTargetLocation,
            const FVector& InTargetVelocity,
            float InProjectileSpeed,
            ECk_Homing_InterceptPreference InPreference)
        -> FCk_Homing_FiringSolution
    {
        CK_ENSURE_IF_NOT(InProjectileSpeed >= 0.0f,
            TEXT("Compute_FiringSolution requires a non-negative ProjectileSpeed, got [{}]"), InProjectileSpeed)
        { return FCk_Homing_FiringSolution{}; }

        const auto MakeSolutionAtTime = [&](double InTime) -> FCk_Homing_FiringSolution
        {
            const auto RelativeVelocity = InTargetVelocity - InShooterVelocity;

            return FCk_Homing_FiringSolution{ECk_SucceededFailed::Succeeded}
                .Set_ImpactLocation(InTargetLocation + RelativeVelocity * InTime)
                .Set_TimeToImpact(FCk_Time{InTime});
        };

        const auto LineOfSight = InTargetLocation - InShooterLocation;

        if (LineOfSight.IsNearlyZero())
        { return MakeSolutionAtTime(0.0); }

        const auto RelativeVelocity = InTargetVelocity - InShooterVelocity;

        if (RelativeVelocity.IsNearlyZero())
        {
            if (FMath::IsNearlyZero(InProjectileSpeed))
            { return FCk_Homing_FiringSolution{}; }

            return MakeSolutionAtTime(LineOfSight.Size() / InProjectileSpeed);
        }

        if (FMath::IsNearlyZero(InProjectileSpeed))
        {
            const auto RelativeSpeed = RelativeVelocity.Size();
            const auto RelativeDirection = RelativeVelocity / RelativeSpeed;
            const auto ProjectionTowardShooter = (-LineOfSight).Dot(RelativeDirection);
            const auto MissDistanceSquared = (-LineOfSight - ProjectionTowardShooter * RelativeDirection).SizeSquared();

            constexpr auto MissToleranceSquared = 1.0e-8;

            const auto TargetMovesTowardShooter = ProjectionTowardShooter >= 0.0;
            const auto TargetPathCrossesShooter = MissDistanceSquared <= MissToleranceSquared;

            if (TargetMovesTowardShooter && TargetPathCrossesShooter)
            { return MakeSolutionAtTime(ProjectionTowardShooter / RelativeSpeed); }

            return FCk_Homing_FiringSolution{};
        }

        // |LOS + Vrel·t| = s·t  ⇔  A·t² + B·t + C = 0
        const auto A = RelativeVelocity.SizeSquared() - FMath::Square(static_cast<double>(InProjectileSpeed));
        const auto B = 2.0 * LineOfSight.Dot(RelativeVelocity);
        const auto C = LineOfSight.SizeSquared();

        const auto QuadraticCollapsesToLinear = FMath::IsNearlyZero(A);

        if (QuadraticCollapsesToLinear)
        {
            if (FMath::IsNearlyZero(B))
            { return FCk_Homing_FiringSolution{}; }

            const auto LinearRoot = -C / B;

            if (LinearRoot < 0.0)
            { return FCk_Homing_FiringSolution{}; }

            return MakeSolutionAtTime(LinearRoot);
        }

        const auto Discriminant = B * B - 4.0 * A * C;

        if (Discriminant < 0.0)
        { return FCk_Homing_FiringSolution{}; }

        const auto SqrtDiscriminant = FMath::Sqrt(Discriminant);
        const auto Root1 = (-B + SqrtDiscriminant) / (2.0 * A);
        const auto Root2 = (-B - SqrtDiscriminant) / (2.0 * A);

        const auto BothRootsValid = Root1 >= 0.0 && Root2 >= 0.0;

        if (BothRootsValid)
        {
            const auto PreferredRoot = InPreference == ECk_Homing_InterceptPreference::EarliestIntercept
                ? FMath::Min(Root1, Root2)
                : FMath::Max(Root1, Root2);

            return MakeSolutionAtTime(PreferredRoot);
        }

        if (Root1 >= 0.0)
        { return MakeSolutionAtTime(Root1); }

        if (Root2 >= 0.0)
        { return MakeSolutionAtTime(Root2); }

        return FCk_Homing_FiringSolution{};
    }
}

// --------------------------------------------------------------------------------------------------------------------
