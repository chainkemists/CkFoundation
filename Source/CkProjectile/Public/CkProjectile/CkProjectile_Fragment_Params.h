#pragma once
#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Params.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment_Params.h"

#include "CkProjectile_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Projectile_ReorientPolicy : uint8
{
    // Projectile rotation will match its velocity direction
    OrientTowardsVelocity,
    DoNotReorient
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Projectile_ReorientPolicy);

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
    ECk_Projectile_ReorientPolicy _ReorientPolicy = ECk_Projectile_ReorientPolicy::OrientTowardsVelocity;

public:
    CK_PROPERTY_GET(_VelocityParams);
    CK_PROPERTY_GET(_AccelerationParams);
    CK_PROPERTY_GET(_ReorientPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Projectile_ParamsData, _VelocityParams, _AccelerationParams, _ReorientPolicy);
};

// --------------------------------------------------------------------------------------------------------------------
