#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkHoming_ProNav.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Homing_SpeedControl : uint8
{
    // Homing acceleration is kept perpendicular to the velocity — direction changes, speed does not.
    // Models fin-steered projectiles coasting after launch
    TurnOnly,

    // Homing may thrust along the line of sight with whatever acceleration budget is left after
    // turning — the projectile speeds up, slows down, or reverses to maximize intercept chance.
    // Models projectiles with an active motor
    TurnAndThrust
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Homing_SpeedControl);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Homing_ImpactTiming : uint8
{
    // Spend all leftover acceleration closing the distance as fast as possible
    ArriveAsap,

    // Modulate the closure rate so the intercept lands when the desired time-to-impact elapses.
    // Subject to the acceleration/speed budget — infeasible timings degrade to best effort
    ArriveAtDesiredTime
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Homing_ImpactTiming);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Homing_InterceptPreference : uint8
{
    EarliestIntercept,
    LatestIntercept
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Homing_InterceptPreference);

// --------------------------------------------------------------------------------------------------------------------

// Snapshot of pursuer + target locations and velocities — the inputs to one guidance evaluation
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_Kinematics
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_Kinematics);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _PursuerLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _PursuerVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TargetLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TargetVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY(_PursuerLocation);
    CK_PROPERTY(_PursuerVelocity);
    CK_PROPERTY(_TargetLocation);
    CK_PROPERTY(_TargetVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Homing_Kinematics, _PursuerLocation, _PursuerVelocity, _TargetLocation, _TargetVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Line-of-sight state derived analytically from a kinematics snapshot — no previous-frame history.
 * The LOS angular velocity is Ω = (R × Vrel) / |R|², the exact instantaneous rotation of the LOS,
 * which makes every downstream guidance computation framerate-independent
 */
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_GuidanceState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_GuidanceState);

private:
    // Unit vector from pursuer to target
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LineOfSightDirection = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    double _LineOfSightDistance = 0.0;

    // Instantaneous rotation of the line of sight in rad/s — always perpendicular to the LOS
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LineOfSightAngularVelocity = FVector::ZeroVector;

    // Rate at which the pursuer closes on the target in cm/s. Negative when receding
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    double _ClosingSpeed = 0.0;

public:
    CK_PROPERTY(_LineOfSightDirection);
    CK_PROPERTY(_LineOfSightDistance);
    CK_PROPERTY(_LineOfSightAngularVelocity);
    CK_PROPERTY(_ClosingSpeed);
};

// --------------------------------------------------------------------------------------------------------------------

// Tuning knobs of the guidance law — lives in homing params and is consumed per-evaluation
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_GuidanceSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_GuidanceSettings);

private:
    // Peak acceleration the guidance may command, in cm/s². The single hard budget every
    // computed acceleration is clamped to
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxAcceleration = 0.0f;

    // How aggressively maneuvers null the LOS rotation. 3-5 is the classical range;
    // 1 degenerates to pure pursuit (tail-chase) and is not recommended
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _NavigationGain = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Homing_SpeedControl _SpeedControl = ECk_Homing_SpeedControl::TurnAndThrust;

    // Speed the pursuer steers toward when it has to come back around after overshooting.
    // 0 = unlimited (thrust straight along the LOS instead). Match the Velocity feature's MaxSpeed
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _MaxSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Homing_ImpactTiming _ImpactTiming = ECk_Homing_ImpactTiming::ArriveAsap;

    // Remaining time in which the intercept should land. Only read when
    // _ImpactTiming == ArriveAtDesiredTime; the caller counts it down between evaluations
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_ImpactTiming == ECk_Homing_ImpactTiming::ArriveAtDesiredTime"))
    FCk_Time _DesiredTimeToImpact;

public:
    CK_PROPERTY_GET(_MaxAcceleration);
    CK_PROPERTY(_NavigationGain);
    CK_PROPERTY(_SpeedControl);
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_ImpactTiming);
    CK_PROPERTY(_DesiredTimeToImpact);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Homing_GuidanceSettings, _MaxAcceleration);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Result of Compute_FiringSolution. ImpactLocation is the world-space point to aim at when firing —
 * for a stationary shooter it is also where the collision happens. For a moving shooter (whose
 * projectile inherits the shooter's velocity) the collision point drifts by ShooterVelocity·T;
 * the aim point is the value to feed into aiming either way
 */
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Homing_FiringSolution
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Homing_FiringSolution);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SucceededFailed _Result = ECk_SucceededFailed::Failed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ImpactLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _TimeToImpact;

public:
    CK_PROPERTY(_Result);
    CK_PROPERTY(_ImpactLocation);
    CK_PROPERTY(_TimeToImpact);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Homing_FiringSolution, _Result);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Proportional Navigation guidance math. Stateless — every function is a pure mapping of its
 * inputs, evaluated identically on every machine at any tick rate. The ECS Homing feature drives
 * these per-frame; tests drive them headlessly.
 *
 * Background: https://en.wikipedia.org/wiki/Proportional_navigation
 */
namespace ck::pronav
{
    // Derives the LOS state from a kinematics snapshot. Zeroed state when pursuer and target coincide
    CKPROJECTILE_API auto
    Compute_GuidanceState(
        const FCk_Homing_Kinematics& InKinematics) -> FCk_Homing_GuidanceState;

    // a = N·Vc·(Ω × R̂) — perpendicular to the LOS. Establishes a collision course; may change
    // the velocity magnitude. Unclamped
    CKPROJECTILE_API auto
    Compute_TrueProNav_Acceleration(
        const FCk_Homing_GuidanceState& InState,
        float InNavigationGain) -> FVector;

    // a = N·(Ω × V) — perpendicular to the velocity. Turns without changing speed. Unclamped
    CKPROJECTILE_API auto
    Compute_PureProNav_Acceleration(
        const FCk_Homing_GuidanceState& InState,
        const FVector& InPursuerVelocity,
        float InNavigationGain) -> FVector;

    /**
     * The complete guidance law: ProNav steering (true or pure per SpeedControl), compensation for
     * the perpendicular component of InExternalAcceleration (gravity sag, wind), leftover-budget
     * thrust along the LOS (TurnAndThrust), optional closure-rate modulation toward a desired
     * time-to-impact, and turn-around recovery when receding. Result magnitude <= MaxAcceleration.
     *
     * InExternalAcceleration is whatever the integrator will apply on top of the returned value —
     * the caller is expected to apply (External + Returned) to the pursuer
     */
    CKPROJECTILE_API auto
    Compute_HomingAcceleration(
        const FCk_Homing_GuidanceState& InState,
        const FCk_Homing_Kinematics& InKinematics,
        const FCk_Homing_GuidanceSettings& InSettings,
        const FVector& InExternalAcceleration,
        FCk_Time InDeltaT) -> FVector;

    /**
     * Constant-velocity intercept solver: where to aim so a projectile fired at InProjectileSpeed
     * meets the target, assuming no drag/gravity. InShooterVelocity is inherited by the projectile
     * (pass zero for emplaced launchers). Handles the degenerate cases exactly: coincident start,
     * stationary target, stationary projectile, |relative speed| == projectile speed (linear root),
     * and both-roots-positive geometries (preference picks earliest/latest)
     */
    CKPROJECTILE_API auto
    Compute_FiringSolution(
        const FVector& InShooterLocation,
        const FVector& InShooterVelocity,
        const FVector& InTargetLocation,
        const FVector& InTargetVelocity,
        float InProjectileSpeed,
        ECk_Homing_InterceptPreference InPreference = ECk_Homing_InterceptPreference::EarliestIntercept) -> FCk_Homing_FiringSolution;
}

// --------------------------------------------------------------------------------------------------------------------
