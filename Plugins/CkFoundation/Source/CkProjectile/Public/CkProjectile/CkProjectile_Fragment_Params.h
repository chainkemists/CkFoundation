#pragma once
#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Params.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment_Params.h"

#include "CkProjectile_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROJECTILE_API FCk_Fragment_Projectile_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Projectile_ParamsData);

public:
    FCk_Fragment_Projectile_ParamsData() = default;
    FCk_Fragment_Projectile_ParamsData(
        FCk_Fragment_Velocity_ParamsData InVelocityParams,
        FCk_Fragment_Acceleration_ParamsData InAccelerationParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Velocity_ParamsData _VelocityParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Fragment_Acceleration_ParamsData _AccelerationParams;

public:
    CK_PROPERTY_GET(_VelocityParams);
    CK_PROPERTY_GET(_AccelerationParams);
};

// --------------------------------------------------------------------------------------------------------------------
