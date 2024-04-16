#pragma once

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Data.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment_Data.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"

#include <InstancedStruct.h>

#include "CkProjectile_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Projectile_AimAhead_Policy : uint8
{
    AimAhead,
    AimAheadPlusOneFrame
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Projectile_AimAhead_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Fragment_Projectile_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Projectile_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_ParamsData _VelocityParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Acceleration_ParamsData _AccelerationParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_AutoReorient_ParamsData _AutoReorientParams;

public:
    CK_PROPERTY_GET(_VelocityParams);
    CK_PROPERTY_GET(_AccelerationParams);
    CK_PROPERTY_GET(_AutoReorientParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Projectile_ParamsData, _VelocityParams, _AccelerationParams, _AutoReorientParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Request_Projectile_CalculateAimAhead
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Projectile_CalculateAimAhead);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Projectile_AimAhead_Policy _AimAheadPolicy = ECk_Projectile_AimAhead_Policy::AimAheadPlusOneFrame;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ProjectileStartingLoc = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ProjectileSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TargetLoc = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _TargetVel = FVector::ZeroVector;

public:
    CK_PROPERTY(_AimAheadPolicy);
    CK_PROPERTY(_ProjectileStartingLoc);
    CK_PROPERTY(_ProjectileSpeed);
    CK_PROPERTY(_TargetLoc);
    CK_PROPERTY(_TargetVel);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Projectile_OnAimAheadCalculated,
    ECk_SucceededFailed, InSuccessFailed,
    const FVector&, InTargetAimPoint,
    const FInstancedStruct&, InOptionalPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Projectile_OnAimAheadCalculated_MC,
    ECk_SucceededFailed, InSuccessFailed,
    const FVector&, InTargetAimPoint,
    const FInstancedStruct&, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------
