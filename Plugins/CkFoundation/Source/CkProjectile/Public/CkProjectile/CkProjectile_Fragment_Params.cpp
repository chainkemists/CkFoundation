#include "CkProjectile_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Projectile_ParamsData::
    FCk_Fragment_Projectile_ParamsData(
        FCk_Fragment_Velocity_ParamsData InVelocityParams,
        FCk_Fragment_Acceleration_ParamsData InAccelerationParams)
    : _VelocityParams(InVelocityParams)
        , _AccelerationParams(InAccelerationParams)
{
}

// --------------------------------------------------------------------------------------------------------------------
