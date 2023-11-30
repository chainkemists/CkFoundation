#pragma once

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Data.h"
#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment_Data.h"
#include "CkPhysics/Velocity/CkVelocity_Fragment_Data.h"

#include "CkProjectile_Fragment_Data.generated.h"

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
