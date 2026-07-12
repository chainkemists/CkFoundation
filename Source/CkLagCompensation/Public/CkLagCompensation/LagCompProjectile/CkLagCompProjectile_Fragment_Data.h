#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment_Data.h"

#include "CkProjectile/BallisticMotion/CkBallisticMotion_Fragment_Data.h"

#include "CkLagCompProjectile_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Launch a ballistic projectile anchored in the PAST (now − CompensationWindow − fudge) and
 * validate the catch-up portion of its trajectory against every RewindHistory entity — the
 * MWO-style server-authoritative hit registration step.
 *
 * The visual catch-up is free: the deterministic trajectory evaluates ahead on the next update,
 * and the projectile's LinearCast probe sweeps the whole catch-up jump for world hits.
 */
USTRUCT(BlueprintType)
struct CKLAGCOMPENSATION_API FCk_Request_LagCompProjectile_LaunchCompensated : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_LagCompProjectile_LaunchCompensated);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_LagCompProjectile_LaunchCompensated);

private:
    // Velocity at launch, in cm/s. The trajectory starts at the entity's current location
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _StartVelocity = FVector::ZeroVector;

    // How far in the past the shot actually happened — typically the shooter's ping.
    // The Ck.LagComp.RewindFudgeFactor CVar is added on top. A negative resolved window is
    // clamped to zero (loudly); rewind sweeps only cover each target's recorded history, so a
    // window beyond the retention period gains nothing
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _CompensationWindow;

    // Radius of the projectile for the rewind sweeps, in cm
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "0", ClampMin = "0"))
    float _ProjectileRadius = 5.0f;

    // Trajectory slice length for the rewind sweeps
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _SubstepTime = FCk_Time{0.05};

    // History entity to exclude from validation — usually the shooter's own
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _IgnoredHistoryEntity;

public:
    CK_PROPERTY_GET(_StartVelocity);
    CK_PROPERTY_GET(_CompensationWindow);
    CK_PROPERTY(_ProjectileRadius);
    CK_PROPERTY(_SubstepTime);
    CK_PROPERTY(_IgnoredHistoryEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_LagCompProjectile_LaunchCompensated, _StartVelocity, _CompensationWindow);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_LagCompProjectile_OnRewindHit,
    FCk_Handle_BallisticMotion, InHandle,
    FCk_LagComp_RewindHit, InHit);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_LagCompProjectile_OnLaunchCompensated,
    FCk_Handle_BallisticMotion, InHandle,
    FCk_Ballistic_InitialConditions, InInitialConditions,
    const TArray<FCk_LagComp_RewindHit>&, InRewindHits);

// --------------------------------------------------------------------------------------------------------------------
