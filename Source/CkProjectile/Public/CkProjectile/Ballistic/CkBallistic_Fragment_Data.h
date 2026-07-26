#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkBallistic_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Flight characteristics for a ballistic trajectory with approximately linear drag, as per
 * Giliam J. P. de Carpentier, "Analytical Ballistic Trajectories with Approximately Linear Drag",
 * International Journal of Computer Games Technology, 2014.
 *
 * The trajectory is a closed-form function of these params + initial conditions + elapsed time.
 * It is NOT integrated frame-by-frame, which makes the path identical on every machine
 * regardless of tick rate — the foundation of lag-compensated projectiles.
 */
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Ballistic_TrajectoryParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ballistic_TrajectoryParams);

private:
    // Velocity at which drag and gravity cancel out, in cm/s. Determines the drag amount.
    // Use a negative Z for regular downward gravity. Must not be zero-length.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TerminalVelocity = FVector{0.0, 0.0, -9000.0};

    // Constant wind velocity in cm/s, applied for the whole trajectory
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Wind = FVector::ZeroVector;

    // Gravitational acceleration in cm/s². Only its magnitude participates in the drag factor;
    // the direction of pull is expressed by _TerminalVelocity
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Gravity = FVector{0.0, 0.0, -980.0};

public:
    // Drag factor k = |Gravity| / (2 * |TerminalVelocity|), as per the Carpentier paper
    auto Get_DragK() const -> double;

    auto Get_TerminalVelocityWithWind() const -> FVector;

public:
    CK_PROPERTY(_TerminalVelocity);
    CK_PROPERTY(_Wind);
    CK_PROPERTY(_Gravity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Ballistic_TrajectoryParams, _TerminalVelocity);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Anchor of one ballistic trajectory segment. A projectile's position/velocity at any moment is a
 * pure function of (InitialConditions, TrajectoryParams, TimeSinceStart). A new segment is anchored
 * whenever the trajectory changes (launch, bounce).
 */
USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Ballistic_InitialConditions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ballistic_InitialConditions);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _StartTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartLocation = FVector::ZeroVector;

    // Velocity at the start of this trajectory segment, in cm/s
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartVelocity = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_StartTime);
    CK_PROPERTY_GET(_StartLocation);
    CK_PROPERTY_GET(_StartVelocity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Ballistic_InitialConditions, _StartTime, _StartLocation, _StartVelocity);
};

// --------------------------------------------------------------------------------------------------------------------
